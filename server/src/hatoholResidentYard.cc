#include <cstdio>
#include <cstdlib>
#include <glib.h>
#include <glib-object.h>
#include <inttypes.h>
#include <dlfcn.h>

#include <Logger.h>
#include <SmartBuffer.h>
using namespace mlpl;

#include "Hatohol.h"
#include "HatoholException.h"
#include "NamedPipe.h"
#include "ResidentProtocol.h"
#include "ResidentCommunicator.h"

struct PrivateContext : public ResidentPullHelper<PrivateContext> {
	GMainLoop *loop;
	NamedPipe pipeRd;
	NamedPipe pipeWr;
	int exitCode;
	void *moduleHandle;
	ResidentModule *module;

	PrivateContext(void)
	: loop(NULL),
	  pipeRd(NamedPipe::END_TYPE_SLAVE_READ),
	  pipeWr(NamedPipe::END_TYPE_SLAVE_WRITE),
	  exitCode(EXIT_SUCCESS),
	  moduleHandle(NULL)
	{
		initResidentPullHelper(&pipeRd, this);
	}

	virtual ~PrivateContext()
	{
		if (loop)
			g_main_loop_unref(loop);
		if (moduleHandle) {
			if (dlclose(moduleHandle) != 0)
				MLPL_ERR("Failed to close module.");
		}
	}
};

static void requestQuit(PrivateContext *ctx, int exitCode = EXIT_FAILURE)
{
	ctx->exitCode = exitCode;
	g_main_loop_quit(ctx->loop);
}

static gboolean readPipeCb
  (GIOChannel *source, GIOCondition condition, gpointer data)
{
	PrivateContext *ctx = static_cast<PrivateContext *>(data);
	MLPL_ERR("Error: condition: %x\n", condition);
	requestQuit(ctx);
	return TRUE;
}

static gboolean writePipeCb
  (GIOChannel *source, GIOCondition condition, gpointer data)
{
	PrivateContext *ctx = static_cast<PrivateContext *>(data);
	MLPL_ERR("Error: condition: %x\n", condition);
	requestQuit(ctx);
	return TRUE;
}

static void eventCb(GIOStatus stat, SmartBuffer &sbuf, size_t size,
                    PrivateContext *ctx);

static void gotNotifyEventBodyCb(GIOStatus stat, mlpl::SmartBuffer &sbuf,
                                 size_t size, PrivateContext *ctx)
{
	ResidentNotifyEventArg arg;
	arg.actionId        = *sbuf.getPointerAndIncIndex<uint32_t>();
	arg.serverId        = *sbuf.getPointerAndIncIndex<uint32_t>();
	arg.hostId          = *sbuf.getPointerAndIncIndex<uint64_t>();
	arg.time.tv_sec     = *sbuf.getPointerAndIncIndex<uint64_t>();
	arg.time.tv_nsec    = *sbuf.getPointerAndIncIndex<uint32_t>();
	arg.eventId         = *sbuf.getPointerAndIncIndex<uint64_t>();
	arg.eventType       = *sbuf.getPointerAndIncIndex<uint16_t>();
	arg.triggerId       = *sbuf.getPointerAndIncIndex<uint64_t>();
	arg.triggerStatus   = *sbuf.getPointerAndIncIndex<uint16_t>();
	arg.triggerSeverity = *sbuf.getPointerAndIncIndex<uint16_t>();

	// call a user action
	uint32_t resultCode = (*ctx->module->notifyEvent)(&arg);
	ResidentCommunicator comm;
	comm.setNotifyEventAck(resultCode);
	comm.push(ctx->pipeWr);

	// request to get the envet
	ctx->pullHeader(eventCb);
}

static void eventCb(GIOStatus stat, SmartBuffer &sbuf, size_t size,
                    PrivateContext *ctx)
{
	if (stat != G_IO_STATUS_NORMAL) {
		MLPL_ERR("Error: status: %x\n", stat);
		requestQuit(ctx);
		return;
	}

	int pktType = ResidentCommunicator::getPacketType(sbuf);
	if (pktType == RESIDENT_PROTO_PKT_TYPE_NOTIFY_EVENT) {
		// request to get the body
		ctx->pullData(RESIDENT_PROTO_EVENT_BODY_LEN,
		              gotNotifyEventBodyCb);
	} else {
		MLPL_ERR("Unexpected packet: %d\n", pktType);
		requestQuit(ctx);
		return;
	}
}

static void sendLaunched(PrivateContext *ctx)
{
	ResidentCommunicator comm;
	comm.setHeader(0, RESIDENT_PROTO_PKT_TYPE_LAUNCHED);
	comm.push(ctx->pipeWr);
}

static void sendModuleLoaded(PrivateContext *ctx)
{
	ResidentCommunicator comm;
	comm.setHeader(0, RESIDENT_PROTO_PKT_TYPE_MODULE_LOADED);
	comm.push(ctx->pipeWr);
}

static void getParametersBodyCb(GIOStatus stat, mlpl::SmartBuffer &sbuf,
                                size_t size, PrivateContext *ctx)
{
	if (stat != G_IO_STATUS_NORMAL) {
		MLPL_ERR("Error: status: %x\n", stat);
		return;
	}

	// length of the module path
	uint16_t modulePathLen = *sbuf.getPointerAndIncIndex<uint16_t>();
	string modulePath(sbuf.getPointer<char>(), modulePathLen);
	sbuf.incIndex(modulePathLen);

	// open the module
	ctx->moduleHandle = dlopen(modulePath.c_str(), RTLD_LAZY);
	if (!ctx->moduleHandle) {
		MLPL_ERR("Failed to load module: %p, %s\n",
		         modulePath.c_str(), dlerror());
		requestQuit(ctx);
		return;
	}

	// get the address of the information structure
	dlerror(); // Clear any existing error
	ctx->module = (ResidentModule *)
	  dlsym(ctx->moduleHandle, RESIDENT_MODULE_SYMBOL_STR);
	char *error;
	if ((error = dlerror()) != NULL) {
		MLPL_ERR("Failed to load symbol: %s, %s\n",
		         RESIDENT_MODULE_SYMBOL_STR, error);
		requestQuit(ctx);
		return;
	}

	// check the module version
	if (ctx->module->moduleVersion != RESIDENT_MODULE_VERSION) {
		MLPL_ERR("Module version unmatched: %"PRIu16", "
		         "expected: %"PRIu16"\n",
		         ctx->module->moduleVersion, RESIDENT_MODULE_VERSION);
		requestQuit(ctx);
		return;
	}

	// check functions
	if (!ctx->module->notifyEvent) {
		MLPL_ERR("notify Event handler is NULL\n");
		requestQuit(ctx);
		return;
	}

	MLPL_INFO("Loaded a resident module: %s\n", modulePath.c_str());

	// send a completion notify
	sendModuleLoaded(ctx);

	// request to get the envet
	ctx->pullHeader(eventCb);
}

static void getParametersCb(GIOStatus stat, mlpl::SmartBuffer &sbuf,
                            size_t size, PrivateContext *ctx)
{
	if (stat != G_IO_STATUS_NORMAL) {
		MLPL_ERR("Error: status: %x\n", stat);
		return;
	}

	// check the packet
	uint32_t bodyLen = *sbuf.getPointerAndIncIndex<uint32_t>();

	// packet type
	uint16_t pktType = *sbuf.getPointerAndIncIndex<uint16_t>();
	if (pktType != RESIDENT_PROTO_PKT_TYPE_PARAMETERS) {
		MLPL_ERR("Invalid packet type: %"PRIu16", "
		         "expect: %"PRIu16"\n", pktType,
		         RESIDENT_PROTO_PKT_TYPE_LAUNCHED);
		requestQuit(ctx);
		return;
	}

	// request to get the body
	ctx->pullData(bodyLen, getParametersBodyCb);
}

static void setupGetParametersCb(PrivateContext *ctx)
{
	ctx->pullHeader(getParametersCb);
}

int mainRoutine(int argc, char *argv[])
{
#ifndef GLIB_VERSION_2_36
	g_type_init();
#endif // GLIB_VERSION_2_36
#ifndef GLIB_VERSION_2_32
	g_thread_init(NULL);
#endif // GLIB_VERSION_2_32 

	hatoholInit();
	PrivateContext ctx;
	MLPL_INFO("started hatohol-resident-yard: ver. %s\n", PACKAGE_VERSION);

	// open pipes
	if (argc < 2) {
		MLPL_ERR("The pipe name is not given. (%d)\n", argc);
		return EXIT_FAILURE;
	}
	const char *pipeName = argv[1];
	MLPL_INFO("PIPE name: %s\n", pipeName);
	if (!ctx.pipeRd.init(pipeName,readPipeCb, &ctx))
		return EXIT_FAILURE;
	if (!ctx.pipeWr.init(pipeName, writePipeCb, &ctx))
		return EXIT_FAILURE;

	sendLaunched(&ctx);
	setupGetParametersCb(&ctx);

	// main loop of GLIB
	ctx.loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(ctx.loop);

	return ctx.exitCode;
}

int main(int argc, char *argv[])
{
	int ret = EXIT_FAILURE;
	try {
		ret = mainRoutine(argc, argv);
	} catch (const HatoholException &e){
		MLPL_ERR("Got exception: %s", e.getFancyMessage().c_str());
	} catch (const exception &e) {
		MLPL_ERR("Got exception: %s", e.what());
	}
	return ret;
}
