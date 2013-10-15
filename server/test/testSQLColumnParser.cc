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

#include <string>
#include <vector>
#include <algorithm>
using namespace std;

#include <ParsableString.h>
#include <StringUtils.h>
using namespace mlpl;

#include <cstdio>
#include <typeinfo>
#include <cutter.h>
#include <cppcutter.h>
#include "SQLColumnParser.h"
#include "FormulaTestUtils.h"
#include "Hatohol.h"
#include "HatoholException.h"

namespace testSQLColumnParser {

static
FormulaVariableDataGetter *columnDataGetter(const string &name, void *priv)
{
	return NULL;
}

static void _assertInputStatement(SQLColumnParser &columnParser,
                                  ParsableString &statement)
{
	SeparatorCheckerWithCallback *separator =
	  columnParser.getSeparatorChecker();
	columnParser.setColumnDataGetterFactory(columnDataGetter, NULL);

	try {
		while (!statement.finished()) {
			string word = statement.readWord(*separator);
			string lower = StringUtils::toLower(word);
			columnParser.add(word, lower);
		}
		columnParser.close();
	} catch (const HatoholException &e) {
		cut_fail("Got exception: %s", e.getFancyMessage().c_str());
	}
}
#define assertInputStatement(P, S) cut_trace(_assertInputStatement(P, S))

#define DEFINE_PARSER_AND_RUN(PTHR, FELEM, STATMNT, NUM_EXP_FORMULA) \
ParsableString _statement(STATMNT); \
SQLColumnParser PTHR; \
PTHR.setColumnDataGetterFactory(columnDataGetter, NULL); \
assertInputStatement(PTHR, _statement); \
const SQLFormulaInfoVector &FELEM = PTHR.getFormulaInfoVector(); \
cppcut_assert_equal(NUM_EXP_FORMULA, formulaInfoVector.size());

void _assertColumn(SQLFormulaInfo *formulaInfo, const char *expectedName)
{
	cppcut_assert_equal(string(expectedName), formulaInfo->expression);
	assertFormulaVariable(formulaInfo->formula, expectedName);
	FormulaVariable *formulaVariable =
	  dynamic_cast<FormulaVariable *>(formulaInfo->formula);
	cut_assert_not_null(formulaVariable);
	cppcut_assert_equal(string(expectedName), formulaVariable->getName());
}
#define assertColumn(X,N) cut_trace(_assertColumn(X,N))

void _assertCount(ParsableString &statement, const char *columnName,
                  bool expectedDistinct)
{
	SQLColumnParser columnParser;
	assertInputStatement(columnParser, statement);

	const SQLFormulaInfoVector formulaInfoVector
	  = columnParser.getFormulaInfoVector();
	cppcut_assert_equal((size_t)1, formulaInfoVector.size());
	SQLFormulaInfo *formulaInfo = formulaInfoVector[0];
	cppcut_assert_equal(string(statement.getString()),
	                    formulaInfo->expression);
	FormulaElement *formulaElem = formulaInfo->formula;
	assertFormulaFuncCount(formulaElem);
	FormulaFuncCount *formulaFuncCount
	  = dynamic_cast<FormulaFuncCount *>(formulaElem);
	cppcut_assert_equal((size_t)1,
	                    formulaFuncCount->getNumberOfArguments());
	FormulaElement *arg = formulaFuncCount->getArgument(0);
	assertFormulaVariable(arg, columnName);
	cppcut_assert_equal(expectedDistinct, formulaFuncCount->isDistinct());
}
#define assertCount(S,C,D) cut_trace(_assertCount(S,C,D))

void _assertSum(ParsableString &statement, const char *columnName)
{
	SQLColumnParser columnParser;
	assertInputStatement(columnParser, statement);

	const SQLFormulaInfoVector formulaInfoVector
	  = columnParser.getFormulaInfoVector();
	cppcut_assert_equal((size_t)1, formulaInfoVector.size());
	SQLFormulaInfo *formulaInfo = formulaInfoVector[0];
	cppcut_assert_equal(string(statement.getString()),
	                    formulaInfo->expression);
	FormulaElement *formulaElem = formulaInfo->formula;
	assertFormulaFuncSum(formulaElem);
	FormulaFuncSum *formulaFuncSum
	  = dynamic_cast<FormulaFuncSum *>(formulaElem);
	cppcut_assert_equal((size_t)1,
	                    formulaFuncSum->getNumberOfArguments());
	FormulaElement *arg = formulaFuncSum->getArgument(0);
	assertFormulaVariable(arg, columnName);
}
#define assertSum(S,C) cut_trace(_assertSum(S,C))

void cut_setup(void)
{
	hatoholInit();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_one(void)
{
	const char *columnName = "column";
	ParsableString statement(StringUtils::sprintf("%s", columnName));
	SQLColumnParser columnParser;
	columnParser.setColumnDataGetterFactory(columnDataGetter, NULL);
	assertInputStatement(columnParser, statement);

	const SQLFormulaInfoVector &formulaInfoVector =
	  columnParser.getFormulaInfoVector();
	cppcut_assert_equal((size_t)1, formulaInfoVector.size());
	assertColumn(formulaInfoVector[0], columnName);
}

void test_oneInteger(void)
{
	int number = 3;
	ParsableString statement(StringUtils::sprintf("%d", number));
	SQLColumnParser columnParser;
	columnParser.setColumnDataGetterFactory(columnDataGetter, NULL);
	assertInputStatement(columnParser, statement);

	const SQLFormulaInfoVector &formulaInfoVector =
	  columnParser.getFormulaInfoVector();
	cppcut_assert_equal((size_t)1, formulaInfoVector.size());
	assertFormulaValue(formulaInfoVector[0]->formula, number);
}

void test_oneDouble(void)
{
	double number = 2.35;
	ParsableString statement(StringUtils::sprintf("%.3lf", number));
	SQLColumnParser columnParser;
	columnParser.setColumnDataGetterFactory(columnDataGetter, NULL);
	assertInputStatement(columnParser, statement);

	const SQLFormulaInfoVector &formulaInfoVector =
	  columnParser.getFormulaInfoVector();
	cppcut_assert_equal((size_t)1, formulaInfoVector.size());
	assertFormulaValue(formulaInfoVector[0]->formula, number);
}

void test_oneDouble(void)
{
	double number = 2.35;
	ParsableString statement(StringUtils::sprintf("%.3lf", number));
	SQLColumnParser columnParser;
	columnParser.setColumnDataGetterFactory(columnDataGetter, NULL);
	assertInputStatement(columnParser, statement);

	const SQLFormulaInfoVector &formulaInfoVector =
	  columnParser.getFormulaInfoVector();
	cppcut_assert_equal((size_t)1, formulaInfoVector.size());
	assertFormulaValue(formulaInfoVector[0]->formula, number);
}

void test_multiColumn(void)
{
	const char *names[] = {"c1", "c2", "c3", "c4", "c5"};
	const size_t numNames = sizeof(names) / sizeof(const char *);
	string str;
	for (size_t i = 0; i < numNames; i++) {
		str += names[i];
		if (i != numNames - 1)
			str += ",";
	}
	ParsableString statement(str.c_str());
	SQLColumnParser columnParser;
	columnParser.setColumnDataGetterFactory(columnDataGetter, NULL);
	assertInputStatement(columnParser, statement);

	const SQLFormulaInfoVector &formulaInfoVector =
	  columnParser.getFormulaInfoVector();
	cppcut_assert_equal(numNames, formulaInfoVector.size());

	for (size_t i = 0; i < numNames; i++) 
		assertColumn(formulaInfoVector[i], names[i]);
}

void test_max(void)
{
	const char *columnName = "c1";
	ParsableString statement(
	  StringUtils::sprintf("max(%s)", columnName));
	SQLColumnParser columnParser;
	assertInputStatement(columnParser, statement);

	const SQLFormulaInfoVector formulaInfoVector
	  = columnParser.getFormulaInfoVector();
	cppcut_assert_equal((size_t)1, formulaInfoVector.size());
	string expected = StringUtils::sprintf("max(%s)", columnName);
	SQLFormulaInfo *formulaInfo = formulaInfoVector[0];
	cppcut_assert_equal(expected, formulaInfo->expression);
	cppcut_assert_equal(true, formulaInfo->hasStatisticalFunc);

	FormulaElement *formulaElem = formulaInfo->formula;
	assertFormulaFuncMax(formulaElem);
	FormulaFuncMax *formulaFuncMax
	  = dynamic_cast<FormulaFuncMax *>(formulaElem);
	cppcut_assert_equal((size_t)1, formulaFuncMax->getNumberOfArguments());
	FormulaElement *arg = formulaFuncMax->getArgument(0);
	assertFormulaVariable(arg, columnName);
}

void test_as(void)
{
	const char *columnName = "c1";
	const char *aliasName = "dog";
	ParsableString statement(
	  StringUtils::sprintf("%s as %s", columnName, aliasName));
	SQLColumnParser columnParser;
	assertInputStatement(columnParser, statement);

	const SQLFormulaInfoVector formulaInfoVector
	  = columnParser.getFormulaInfoVector();
	cppcut_assert_equal((size_t)1, formulaInfoVector.size());
	SQLFormulaInfo *formulaInfo = formulaInfoVector[0];
	cppcut_assert_equal(string(columnName), formulaInfo->expression);
	cppcut_assert_equal(string(aliasName), formulaInfo->alias);

	FormulaElement *formulaElem = formulaInfo->formula;
	assertFormulaVariable(formulaElem, columnName);
}

void test_count(void)
{
	const char *columnName = "c1";
	ParsableString statement(
	  StringUtils::sprintf("count(%s)", columnName));
	bool distinct = false;
	assertCount(statement, columnName, distinct);
}

void test_sum(void)
{
	const char *columnName = "c1";
	ParsableString statement(
	  StringUtils::sprintf("sum(%s)", columnName));
	assertSum(statement, columnName);
}

void test_countDistinct(void)
{
	const char *columnName = "c1";
	ParsableString statement(
	  StringUtils::sprintf("count(distinct %s)", columnName));
	bool distinct = true;
	assertCount(statement, columnName, distinct);
}

void test_doubleDivVar(void)
{
	double number = 3.3625;
	const char *columnName = "fooColumn";
	const size_t expectedNumFormula = 1;
	string statement = StringUtils::sprintf("%.4lf/%s", number, columnName);
	DEFINE_PARSER_AND_RUN(columnParser, formulaInfoVector, statement,
	                      expectedNumFormula);
	FormulaElement *rootElem = formulaInfoVector[0]->formula;
	assertFormulaOperatorDiv(rootElem);
	assertFormulaValue(rootElem->getLeftHand(), number);
	assertFormulaVariable(rootElem->getRightHand(), columnName);
}

void test_sumDoubleDivVar(void)
{
	double number = 3.3625;
	const char *columnName = "fooColumn";
	const size_t expectedNumFormula = 1;
	string statement = StringUtils::sprintf("sum(%.4lf/%s)", 
	                                        number, columnName);
	DEFINE_PARSER_AND_RUN(columnParser, formulaInfoVector, statement,
	                      expectedNumFormula);
	FormulaElement *rootElem = formulaInfoVector[0]->formula;
	assertFormulaFuncSum(rootElem);

	FormulaElement *innerElem = rootElem->getLeftHand();
	assertFormulaOperatorDiv(innerElem);
	assertFormulaValue(innerElem->getLeftHand(), number);
	assertFormulaVariable(innerElem->getRightHand(), columnName);
}

} // namespace testSQLColumnParser

