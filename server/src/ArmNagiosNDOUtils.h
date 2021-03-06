/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ArmNagiosNDOUtils_h
#define ArmNagiosNDOUtils_h

#include <libsoup/soup.h>
#include "ArmBase.h"
#include "ItemTablePtr.h"
#include "JsonParserAgent.h"
#include "JsonBuilderAgent.h"
#include "DBClientConfig.h"
#include "DBClientZabbix.h"

class ArmNagiosNDOUtils : public ArmBase
{
public:
	ArmNagiosNDOUtils(const MonitoringServerInfo &serverInfo);
	virtual ~ArmNagiosNDOUtils();

protected:
	void makeSelectTriggerArg(void);
	void makeSelectEventArg(void);
	void makeSelectItemArg(void);
	void addConditionForTriggerQuery(void);
	void addConditionForEventQuery(void);
	void getTrigger(void);
	void getEvent(void);
	void getItem(void);
	void connect(void);

	// virtual methods
	virtual gpointer mainThread(HatoholThreadArg *arg);
	virtual bool mainThreadOneProc(void);

private:
	struct PrivateContext;
	PrivateContext *m_ctx;
};

#endif // ArmNagiosNDOUtils_h
