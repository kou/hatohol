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

#ifndef DBAgentTestCommon_h
#define DBAgentTestCommon_h

#include <cppcutter.h>
#include "SQLProcessorTypes.h"
#include "DBAgent.h"

extern const char *TABLE_NAME_TEST;
extern const ColumnDef COLUMN_DEF_TEST[];
extern const size_t NUM_COLUMNS_TEST;

enum {
	IDX_TEST_TABLE_ID,
	IDX_TEST_TABLE_AGE,
	IDX_TEST_TABLE_NAME,
	IDX_TEST_TABLE_HEIGHT,
	IDX_TEST_TABLE_TIME,
};

extern const size_t NUM_TEST_DATA;
extern const uint64_t ID[];
extern const int AGE[];
extern const char *NAME[];
extern const double HEIGHT[];
extern const int    TIME[];

extern const int MAX_ALLOWD_CURR_TIME_ERROR;

class DBAgentChecker {
public:
	virtual void assertTable(const DBAgentTableCreationArg &arg) = 0;
	virtual void assertExistingRecord(
	               uint64_t id, int age, const char *name, double height,
	               int datetime, size_t numColumns,
	               const ColumnDef *columnDefs,
	               const set<size_t> *nullIndexes = NULL) = 0;
	virtual void getIDStringVector(const ColumnDef &columnDefId,
	                               vector<string> &actualIds) = 0;

	static void createTable(DBAgent &dbAgent);
	static void insert(DBAgent &dbAgent, uint64_t id, int age,
	                   const char *name, double height, int time);
	static void makeTestData(DBAgent &dbAgent);
	static void makeTestData(DBAgent &dbAgent,
	                         map<uint64_t, size_t> &testDataIdIndexMap);
protected:
	static void assertExistingRecordEachWord
	              (uint64_t id, int age, const char *name, double height,
	               int datetime, size_t numColumns,
	               const ColumnDef *columnDefs, const string &line,
	               const char splitChar,
	               const set<size_t> *nullIndexes,
	               const string &expectedNullNotation,
	               const char *U64fmt = "%"PRIu64);
};

void dbAgentTestCreateTable(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestInsert(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestInsertUint64
  (DBAgent &dbAgent, DBAgentChecker &checker, uint64_t id);
void dbAgentTestInsertNull(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestUpdate(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestUpdateCondition(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestSelect(DBAgent &dbAgent);
void dbAgentTestSelectEx(DBAgent &dbAgent);
void dbAgentTestSelectExWithCond(DBAgent &dbAgent);
void dbAgentTestSelectExWithCondAllColumns(DBAgent &dbAgent);
void dbAgentTestSelectHeightOrder
 (DBAgent &dbAgent, size_t limit = 0, size_t offset = 0,
  size_t forceExpectedRows = (size_t)-1);
void dbAgentTestDelete(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestIsTableExisting(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestAutoIncrement(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentTestAutoIncrementWithDel(DBAgent &dbAgent, DBAgentChecker &checker);
void dbAgentUpdateIfExistEleseInsert(DBAgent &dbAgent, DBAgentChecker &checker);

#endif // DBAgentTestCommon_h
