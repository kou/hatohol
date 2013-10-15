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
#include "SQLWhereParser.h"
#include "FormulaOperator.h"
#include "FormulaTestUtils.h"
#include "Hatohol.h"
#include "HatoholException.h"
#include "PrimaryConditionPicker.h"

namespace testSQLWhereParser {

static
FormulaVariableDataGetter *columnDataGetter(const string &name, void *priv)
{
	return NULL;
}

class SQLProcessorSelectFactoryImpl : public SQLProcessorSelectFactory
{
	virtual SQLProcessorSelect *create(SQLSubQueryMode subQueryMode) {
		return NULL;
	}
};

static SQLProcessorSelectFactoryImpl procSelectFactory;
static SQLProcessorSelectShareInfo shareInfo(procSelectFactory);

static void _assertInputStatement(SQLWhereParser &whereParser,
                                  ParsableString &statement)
{
	SeparatorCheckerWithCallback *separator =
	  whereParser.getSeparatorChecker();
	whereParser.setColumnDataGetterFactory(columnDataGetter, NULL);
	shareInfo.statement = &statement;
	whereParser.setShareInfo(&shareInfo);

	try {
		while (!statement.finished()) {
			string word = statement.readWord(*separator);
			string lower = StringUtils::toLower(word);
			whereParser.add(word, lower);
		}
		whereParser.close();
	} catch (const HatoholException &e) {
		cut_fail("Got exception: %s", e.getFancyMessage().c_str());
	}
}
#define assertInputStatement(P, S) cut_trace(_assertInputStatement(P, S))

#define DEFINE_PARSER_AND_RUN(WPTHR, FELEM, STATMNT) \
ParsableString _statement(STATMNT); \
SQLWhereParser WPTHR; \
assertInputStatement(WPTHR, _statement); \
FormulaElement *FELEM = WPTHR.getFormula(); \
cppcut_assert_not_null(FELEM);

template<typename T, typename VT>
static void _assertWhereIn(T *expectedValueArray, size_t numValue)
{
	const char *columnName = "testColumnForWhere";
	vector<VT> expectedValues;
	stringstream statementStream;
	statementStream << columnName;
	statementStream << " in (";
	for (size_t i = 0; i < numValue; i++) {
		T &v = expectedValueArray[i];
		statementStream << "'";
		statementStream << v;
		statementStream << "'";
		if (i < numValue - 1)
			statementStream << ",";
		else
			statementStream << ")";

		expectedValues.push_back(v);
	}

	ParsableString statement(statementStream.str());
	SQLWhereParser whereParser;
	assertInputStatement(whereParser, statement);

	FormulaElement *formula = whereParser.getFormula();
	assertFormulaInWithVarName(formula, expectedValues, columnName);
}
#define assertWhereIn(T, VT, EXP, NUM) \
cut_trace((_assertWhereIn<T, VT>(EXP, NUM)))

static void _assertPrimaryConditionColumnsEqual
  (const PrimaryCondition *primaryCondition, 
   const string &leftTable, const string &leftColumn,
   const string &rightTable, const string &rightColumn)
{
	const PrimaryConditionColumnsEqual *condColumnsEqual =
	  dynamic_cast<const PrimaryConditionColumnsEqual *>(primaryCondition);
	cppcut_assert_not_null(condColumnsEqual);
	cppcut_assert_equal(leftTable,   condColumnsEqual->getLeftTableName());
	cppcut_assert_equal(leftColumn,  condColumnsEqual->getLeftColumnName());
	cppcut_assert_equal(rightTable,  condColumnsEqual->getRightTableName());
	cppcut_assert_equal(rightColumn, condColumnsEqual->getRightColumnName());
}
#define assertPrimaryConditionColumnsEqual(CCI, LT, LC, RT, RC) \
cut_trace((_assertPrimaryConditionColumnsEqual(CCI, LT, LC, RT, RC)))

void cut_setup(void)
{
	hatoholInit();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_whereEqNumber(void)
{
	const char *leftHand = "a";
	int rightHand = 1;
	ParsableString statement(
	  StringUtils::sprintf("%s=%d", leftHand, rightHand));
	SQLWhereParser whereParser;
	assertInputStatement(whereParser, statement);
	FormulaElement *formula = whereParser.getFormula();
	assertFormulaComparatorEqual(formula);
	assertFormulaVariable(formula->getLeftHand(), leftHand);
	assertFormulaValue(formula->getRightHand(), rightHand);
}

void test_whereEqString(void)
{
	const char *leftHand = "a";
	const char *rightHand = "foo XYZ";
	ParsableString statement(
	  StringUtils::sprintf("%s='%s'", leftHand, rightHand));
	SQLWhereParser whereParser;
	assertInputStatement(whereParser, statement);
	FormulaElement *formula = whereParser.getFormula();
	assertFormulaComparatorEqual(formula);
	assertFormulaVariable(formula->getLeftHand(), leftHand);
	assertFormulaValue(formula->getRightHand(), rightHand);
}

void test_whereEqColumnColumn(void)
{
	const char *leftHand = "a";
	const char *rightHand = "b";
	ParsableString statement(
	  StringUtils::sprintf("%s=%s", leftHand, rightHand));
	SQLWhereParser whereParser;
	assertInputStatement(whereParser, statement);
	FormulaElement *formula = whereParser.getFormula();
	assertFormulaComparatorEqual(formula);
	assertFormulaVariable(formula->getLeftHand(), leftHand);
	assertFormulaVariable(formula->getRightHand(), rightHand);
}

void test_whereAnd(void)
{
	const char *leftHand0  = "a";
	int         rightHand0 = 1;
	const char *leftHand1  = "b";
	const char *rightHand1 = "foo";
	ParsableString statement(
	  StringUtils::sprintf("%s=%d and %s='%s'",
	                       leftHand0, rightHand0, leftHand1, rightHand1));
	SQLWhereParser whereParser;
	assertInputStatement(whereParser, statement);

	FormulaElement *formula = whereParser.getFormula();
	assertFormulaOperatorAnd(formula);

	// left child
	FormulaElement *leftElem = formula->getLeftHand();
	assertFormulaComparatorEqual(leftElem);
	assertFormulaVariable(leftElem->getLeftHand(), leftHand0);
	assertFormulaValue(leftElem->getRightHand(), rightHand0);

	// right child
	FormulaElement *rightElem = formula->getRightHand();
	assertFormulaComparatorEqual(rightElem);
	assertFormulaVariable(rightElem->getLeftHand(), leftHand1);
	assertFormulaValue(rightElem->getRightHand(), rightHand1);
}

void test_whereBetween(void)
{
	const char *leftHand = "a";
	int v0 = 0;
	int v1 = 100;
	ParsableString statement(
	  StringUtils::sprintf("%s between %d and %d", leftHand, v0, v1));
	SQLWhereParser whereParser;
	assertInputStatement(whereParser, statement);

	FormulaElement *formula = whereParser.getFormula();
	assertFormulaBetweenWithVarName(formula, v0, v1, leftHand);
}

void test_whereInInt(void)
{
	int v[] = {5};
	const size_t numValue = sizeof(v) / sizeof(int);
	assertWhereIn(int, int, v, numValue);
}

void test_whereInMultipleInt(void)
{
	int v[] = {5, -8, 200};
	const size_t numValue = sizeof(v) / sizeof(int);
	assertWhereIn(int, int, v, numValue);
}

void test_whereInString(void)
{
	const char *v[] = {"foo goo XYZ, y = f(x)"};
	const size_t numValue = sizeof(v) / sizeof(const char *);
	assertWhereIn(const char *, string, v, numValue);
}

void test_whereInMultipleString(void)
{
	const char *v[] = {"A A A", "<x,y> = (8,5)", "Isaac Newton"};
	const size_t numValue = sizeof(v) / sizeof(const char *);
	assertWhereIn(const char *, string, v, numValue);
}

void test_whereInMultipleInt(void)
{
	const char *leftHand = "a";
	int v[] = {5, -8, 200};
	const size_t numValue = sizeof(v) / sizeof(int);
	ParsableString statement(
	  StringUtils::sprintf("%s in ('%d', '%d', '%d')",
	                       leftHand, v[0], v[1], v[2]));
	SQLWhereParser whereParser;
	assertInputStatement(whereParser, statement);

	vector<int> expectedValues;
	for (size_t i = 0; i < numValue; i++)
		expectedValues.push_back(v[i]);
	FormulaElement *formula = whereParser.getFormula();
	assertFormulaInWithVarName(formula, expectedValues, leftHand);
}

void test_whereInString(void)
{
	const char *leftHand = "a";
	const char *v0 = "foo goo XYZ, y = f(x)";
	ParsableString statement(
	  StringUtils::sprintf("%s in ('%s')", leftHand, v0));
	SQLWhereParser whereParser;
	assertInputStatement(whereParser, statement);

	vector<string> expectedValues;
	expectedValues.push_back(v0);
	FormulaElement *formula = whereParser.getFormula();
	assertFormulaInWithVarName(formula, expectedValues, leftHand);
}

void test_whereIntAndStringAndInt(void)
{
	const char *leftHand0  = "a";
	int         rightHand0 = 1;
	const char *leftHand1  = "b";
	const char *rightHand1 = "foo";
	const char *leftHand2  = "c";
	int         rightHand2 = -5;
	ParsableString statement(
	  StringUtils::sprintf("%s=%d and %s='%s' and %s=%d",
	                       leftHand0, rightHand0, leftHand1, rightHand1,
	                       leftHand2, rightHand2));
	SQLWhereParser whereParser;
	assertInputStatement(whereParser, statement);

	FormulaElement *formula = whereParser.getFormula();
	assertFormulaOperatorAnd(formula);

	FormulaElement *leftElem = formula->getLeftHand();
	FormulaElement *rightElem = formula->getRightHand();
	assertFormulaComparatorEqual(leftElem);
	assertFormulaOperatorAnd(rightElem);

	// Top left child
	assertFormulaVariable(leftElem->getLeftHand(), leftHand0);
	assertFormulaValue(leftElem->getRightHand(), rightHand0);

	// Top right child
	assertFormulaComparatorEqual(rightElem->getLeftHand());
	assertFormulaComparatorEqual(rightElem->getRightHand());

	// Second level left
	leftElem = rightElem->getLeftHand();
	assertFormulaVariable(leftElem->getLeftHand(), leftHand1);
	assertFormulaValue(leftElem->getRightHand(), rightHand1);

	// Second level child
	rightElem = rightElem->getRightHand();
	assertFormulaVariable(rightElem->getLeftHand(), leftHand2);
	assertFormulaValue(rightElem->getRightHand(), rightHand2);
}

void test_whereIntAndColumnAndInt(void)
{
	const char *leftHand0  = "a";
	int         rightHand0 = 1;
	const char *leftHand1  = "b";
	const char *rightHand1 = "x";
	const char *leftHand2  = "c";
	int         rightHand2 = -5;
	ParsableString statement(
	  StringUtils::sprintf("%s=%d and %s=%s and %s=%d",
	                       leftHand0, rightHand0, leftHand1, rightHand1,
	                       leftHand2, rightHand2));
	SQLWhereParser whereParser;
	assertInputStatement(whereParser, statement);

	FormulaElement *formula = whereParser.getFormula();
	assertFormulaOperatorAnd(formula);

	FormulaElement *leftElem = formula->getLeftHand();
	FormulaElement *rightElem = formula->getRightHand();
	assertFormulaComparatorEqual(leftElem);
	assertFormulaOperatorAnd(rightElem);

	// Top left child
	assertFormulaVariable(leftElem->getLeftHand(), leftHand0);
	assertFormulaValue(leftElem->getRightHand(), rightHand0);

	// Top right child
	assertFormulaComparatorEqual(rightElem->getLeftHand());
	assertFormulaComparatorEqual(rightElem->getRightHand());

	// Second level left
	leftElem = rightElem->getLeftHand();
	assertFormulaVariable(leftElem->getLeftHand(), leftHand1);
	assertFormulaVariable(leftElem->getRightHand(), rightHand1);

	// Second level child
	rightElem = rightElem->getRightHand();
	assertFormulaVariable(rightElem->getLeftHand(), leftHand2);
	assertFormulaValue(rightElem->getRightHand(), rightHand2);
}

void test_intAndBetween(void)
{
	const char *leftHand0  = "a";
	int         rightHand0 = 1;

	const char *btwVar = "b";
	int btwVal0 = 0;
	int btwVal1 = 100;
	string statement =
	  StringUtils::sprintf("%s=%d and %s between %d and %d",
	                       leftHand0, rightHand0,
	                       btwVar, btwVal0, btwVal1);
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	assertFormulaOperatorAnd(formula);

	FormulaElement *leftElem = formula->getLeftHand();
	FormulaElement *rightElem = formula->getRightHand();
	assertFormulaComparatorEqual(leftElem);
	assertFormulaVariable(leftElem->getLeftHand(), leftHand0);
	assertFormulaValue(leftElem->getRightHand(), rightHand0);
	assertFormulaBetweenWithVarName(rightElem, btwVal0, btwVal1, btwVar);
}

void test_intAndBetweenThenParenthesisClose(void)
{
	const char *btwVar = "b";
	int btwVal0 = 0;
	int btwVal1 = 100;
	string statement =
	  StringUtils::sprintf("(%s between %d and %d)",
	                       btwVar, btwVal0, btwVal1);
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	assertFormulaParenthesis(formula);
	FormulaElement *leftElem = formula->getLeftHand();
	assertFormulaBetweenWithVarName(leftElem, btwVal0, btwVal1, btwVar);
}

void test_parenthesis(void)
{
	const char *elemName0  = "a";
	const char *elemName1  = "b";
	const char *elemName2  = "c";
	string statement =
	  StringUtils::sprintf("(%s or %s) and %s",
	                       elemName0, elemName1, elemName2);
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	assertFormulaOperatorAnd(formula);

	// In parenthesis
	FormulaElement *leftElem = formula->getLeftHand();
	assertFormulaParenthesis(leftElem);

	leftElem = leftElem->getLeftHand();
	assertFormulaOperatorOr(leftElem);
	assertFormulaVariable(leftElem->getLeftHand(), elemName0);
	assertFormulaVariable(leftElem->getRightHand(), elemName1);

	// right hand
	FormulaElement *rightElem = formula->getRightHand();
	assertFormulaVariable(rightElem, elemName2);
}

void test_parenthesisDouble(void)
{
	const char *elemName0  = "a";
	const char *elemName1  = "b";
	const char *elemName2  = "c";
	const char *elemName3  = "d";
	string statement =
	  StringUtils::sprintf("((%s or %s) and %s) or %s",
	                       elemName0, elemName1, elemName2, elemName3);
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	assertFormulaOperatorOr(formula);

	// In the outer parenthesis
	FormulaElement *outerParenElem = formula->getLeftHand();
	assertFormulaParenthesis(outerParenElem);

	FormulaElement *andElem = outerParenElem->getLeftHand();
	assertFormulaOperatorAnd(andElem);

	// Hands of 'and'
	FormulaElement *innerParenElem = andElem->getLeftHand();
	assertFormulaParenthesis(innerParenElem);

	FormulaElement *orElem = innerParenElem->getLeftHand();
	assertFormulaOperatorOr(orElem);
	assertFormulaVariable(orElem->getLeftHand(), elemName0);
	assertFormulaVariable(orElem->getRightHand(), elemName1);

	// The element before closing parenthesis
	assertFormulaVariable(andElem->getRightHand(), elemName2);

	// The most right hand
	assertFormulaVariable(formula->getRightHand(), elemName3);
}

void test_plus(void)
{
	const char *elemName0  = "a";
	const char *elemName1  = "b";
	const char *elemName2  = "c";
	string statement =
	  StringUtils::sprintf("%s + %s = %s",
	                       elemName0, elemName1, elemName2);
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	assertFormulaComparatorEqual(formula);

	FormulaElement *plusElem = formula->getLeftHand();
	assertFormulaOperatorPlus(plusElem);
	assertFormulaVariable(plusElem->getLeftHand(), elemName0);
	assertFormulaVariable(plusElem->getRightHand(), elemName1);
	assertFormulaVariable(formula->getRightHand(), elemName2);
}

void test_greaterThan(void)
{
	const char *elemName0  = "a";
	const char *elemName1  = "b";
	string statement =
	  StringUtils::sprintf("%s > %s", elemName0, elemName1);
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	assertFormulaGreaterThan(formula);
	assertFormulaVariable(formula->getLeftHand(), elemName0);
	assertFormulaVariable(formula->getRightHand(), elemName1);
}

void test_greaterOrEqual(void)
{
	const char *elemName0  = "a";
	const char *elemName1  = "b";
	string statement =
	  StringUtils::sprintf("%s >= %s", elemName0, elemName1);
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	assertFormulaGreaterOrEqual(formula);
	assertFormulaVariable(formula->getLeftHand(), elemName0);
	assertFormulaVariable(formula->getRightHand(), elemName1);
}

void test_notEqGtLt(void)
{
	const char *elemName0  = "a";
	const char *elemName1  = "b";
	string statement =
	  StringUtils::sprintf("%s <> %s", elemName0, elemName1);
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	assertFormulaComparatorNotEqual(formula);
	assertFormulaVariable(formula->getLeftHand(), elemName0);
	assertFormulaVariable(formula->getRightHand(), elemName1);
}

void test_exists(void)
{
	const char *innerSelect = "select ((a/3)*2+8) from b where (x+5*c)=d";
	string statement =
	  StringUtils::sprintf("exists (%s)", innerSelect);
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	FormulaExists *formulaExists = dynamic_cast<FormulaExists *>(formula);
	cppcut_assert_equal(string(innerSelect), formulaExists->getStatement());
}

void test_not(void)
{
	const char *elemName0 = "a";
	string statement = StringUtils::sprintf("not %s", elemName0);
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	assertFormulaOperatorNot(formula);
	assertFormulaVariable(formula->getLeftHand(), elemName0);
}

void test_isNull(void)
{
	const char *elemName0 = "a";
	string statement = StringUtils::sprintf("%s is null", elemName0);
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	assertTypeFormulaIsNull(formula);
	assertFormulaVariable(formula->getLeftHand(), elemName0);
}

void test_isNotNull(void)
{
	const char *elemName0 = "a";
	string statement = StringUtils::sprintf("%s is not null", elemName0);
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	assertTypeFormulaIsNotNull(formula);
	assertFormulaVariable(formula->getLeftHand(), elemName0);
}

//
// optimize
//
void test_optimize1Eq1(void)
{
	string statement = StringUtils::sprintf("1=1");
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	assertFormulaComparatorEqual(formula);
	FormulaOptimizationResult result = formula->optimize();
	cppcut_assert_equal(FORMULA_ALWAYS_TRUE, result.type);
}

void test_optimize1Eq0(void)
{
	string statement = StringUtils::sprintf("1=0");
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	assertFormulaComparatorEqual(formula);
	FormulaOptimizationResult result = formula->optimize();
	cppcut_assert_equal(FORMULA_ALWAYS_FALSE, result.type);
}

void test_optimize1Eq1And1Eq1(void)
{
	string statement = StringUtils::sprintf("1=1 and 1=1");
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	assertFormulaOperatorAnd(formula);
	FormulaOptimizationResult result = formula->optimize();
	cppcut_assert_equal(FORMULA_ALWAYS_TRUE, result.type);
}

void test_optimize1Eq1AndVar(void)
{
	string statement = StringUtils::sprintf("1=1 and var");
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	assertFormulaOperatorAnd(formula);
	FormulaOptimizationResult result = formula->optimize();
	cppcut_assert_equal(FORMULA_UNFIXED, result.type);
}

void test_optimize1Eq0AndVar(void)
{
	string statement = StringUtils::sprintf("1=0 and var");
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	assertFormulaOperatorAnd(formula);
	FormulaOptimizationResult result = formula->optimize();
	cppcut_assert_equal(FORMULA_ALWAYS_FALSE, result.type);
}

void test_optimize1Eq0And1Eq1(void)
{
	string statement = StringUtils::sprintf("1=0 and 1=1");
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	assertFormulaOperatorAnd(formula);
	FormulaOptimizationResult result = formula->optimize();
	cppcut_assert_equal(FORMULA_ALWAYS_FALSE, result.type);
}

//
// PrimaryConditionPicker
//
void test_columnComparisonOne(void)
{
	const char *leftTable = "a";
	const char *leftColumn = "c1";
	const char *rightTable = "b";
	const char *rightColumn = "c2";
	string statement =
	   StringUtils::sprintf("%s.%s=%s.%s",
	                        leftTable, leftColumn, rightTable, rightColumn);
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	PrimaryConditionPicker columnCompPicker;
	columnCompPicker.pickupPrimary(formula);
	const PrimaryConditionList &columnCompInfoList = 
	  columnCompPicker.getPrimaryConditionList();
	cppcut_assert_equal((size_t)1, columnCompInfoList.size());
	assertPrimaryConditionColumnsEqual(*columnCompInfoList.begin(),
	                                   leftTable, leftColumn,
	                                   rightTable, rightColumn);
}

void test_columnComparisonTwoEq(void)
{
	const char *leftTable0 = "a";
	const char *leftColumn0 = "c1";
	const char *rightTable0 = "b";
	const char *rightColumn0 = "c2";
	const char *leftTable1 = "udon";
	const char *leftColumn1 = "dog";
	const char *rightTable1 = "soba";
	const char *rightColumn1 = "cat";
	string statement =
	   StringUtils::sprintf(
	     "%s.%s=%s.%s and %s.%s=%s.%s",
	     leftTable0, leftColumn0, rightTable0, rightColumn0,
	     leftTable1, leftColumn1, rightTable1, rightColumn1);
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	PrimaryConditionPicker columnCompPicker;
	columnCompPicker.pickupPrimary(formula);
	const PrimaryConditionList &columnCompInfoList = 
	  columnCompPicker.getPrimaryConditionList();
	cppcut_assert_equal((size_t)2, columnCompInfoList.size());

	PrimaryConditionListConstIterator it = columnCompInfoList.begin();
	assertPrimaryConditionColumnsEqual(*it,
	                                   leftTable0, leftColumn0,
	                                   rightTable0, rightColumn0);
	++it;
	assertPrimaryConditionColumnsEqual(*it,
	                                   leftTable1, leftColumn1,
	                                   rightTable1, rightColumn1);
}

void test_columnComparisonThreeComp(void)
{
	const char *leftTable0 = "a";
	const char *leftColumn0 = "c1";
	const char *rightTable0 = "b";
	const char *rightColumn0 = "c2";
	const char *leftTable1 = "udon";
	const char *leftColumn1 = "dog";
	const char *rightTable1 = "soba";
	const char *rightColumn1 = "cat";
	const char *leftTable2 = "fish";
	const char *leftColumn2 = "red";
	const char *rightTable2 = "cow";
	const char *rightColumn2 = "black";
	string statement =
	   StringUtils::sprintf(
	     "%s.%s=%s.%s and %s.%s=%s.%s and %s.%s=%s.%s",
	     leftTable0, leftColumn0, rightTable0, rightColumn0,
	     leftTable1, leftColumn1, rightTable1, rightColumn1,
	     leftTable2, leftColumn2, rightTable2, rightColumn2);
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	PrimaryConditionPicker columnCompPicker;
	columnCompPicker.pickupPrimary(formula);
	const PrimaryConditionList &columnCompInfoList = 
	  columnCompPicker.getPrimaryConditionList();
	cppcut_assert_equal((size_t)3, columnCompInfoList.size());

	PrimaryConditionListConstIterator it = columnCompInfoList.begin();
	assertPrimaryConditionColumnsEqual(*it,
	                                   leftTable0, leftColumn0,
	                                   rightTable0, rightColumn0);
	++it;
	assertPrimaryConditionColumnsEqual(*it,
	                                   leftTable1, leftColumn1,
	                                   rightTable1, rightColumn1);
	++it;
	assertPrimaryConditionColumnsEqual(*it,
	                                   leftTable2, leftColumn2,
	                                   rightTable2, rightColumn2);
}

void test_columnComparisonOr(void)
{
	string statement = "A.C1=B.C1 or C.C2=D.C2";
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	PrimaryConditionPicker columnCompPicker;
	columnCompPicker.pickupPrimary(formula);
	const PrimaryConditionList &columnCompInfoList = 
	  columnCompPicker.getPrimaryConditionList();
	cppcut_assert_equal((size_t)0, columnCompInfoList.size());
}

void test_columnComparisonAndOr(void)
{
	const char *leftTable = "a";
	const char *leftColumn = "c1";
	const char *rightTable = "b";
	const char *rightColumn = "c2";
	string statement =
	   StringUtils::sprintf("%s.%s=%s.%s and (A.C1=B.C1 or C.C2=D.C2)",
	                        leftTable, leftColumn, rightTable, rightColumn);
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	PrimaryConditionPicker columnCompPicker;
	columnCompPicker.pickupPrimary(formula);
	const PrimaryConditionList &columnCompInfoList = 
	  columnCompPicker.getPrimaryConditionList();
	cppcut_assert_equal((size_t)1, columnCompInfoList.size());
	assertPrimaryConditionColumnsEqual(*columnCompInfoList.begin(),
	                                   leftTable, leftColumn,
	                                   rightTable, rightColumn);
}

void test_columnComparisonOrAnd(void)
{
	const char *leftTable = "a";
	const char *leftColumn = "c1";
	const char *rightTable = "b";
	const char *rightColumn = "c2";
	string statement =
	   StringUtils::sprintf("(A.C1=B.C1 or C.C2=D.C2) and %s.%s=%s.%s",
	                        leftTable, leftColumn, rightTable, rightColumn);
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	PrimaryConditionPicker columnCompPicker;
	columnCompPicker.pickupPrimary(formula);
	const PrimaryConditionList &columnCompInfoList = 
	  columnCompPicker.getPrimaryConditionList();
	cppcut_assert_equal((size_t)1, columnCompInfoList.size());
	assertPrimaryConditionColumnsEqual(*columnCompInfoList.begin(),
	                                   leftTable, leftColumn,
	                                   rightTable, rightColumn);
}

void test_removeParenthesis(void)
{
	const char *eqColumn0 = "a";
	const char *eqColumn1 = "b";
	const char *inColumn  = "c";
	string statement =
	   StringUtils::sprintf("(%s=%s) AND (%s IN ('1','3'))",
	                        eqColumn0, eqColumn1, inColumn);
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	formula->removeParenthesis();

	assertFormulaOperatorAnd(formula);
	FormulaElement *leftHand = formula->getLeftHand();
	assertFormulaComparatorEqual(leftHand);
	assertFormulaVariable(leftHand->getLeftHand(), eqColumn0);
	assertFormulaVariable(leftHand->getRightHand(), eqColumn1);

	assertTypeFormulaIn(formula->getRightHand());
}

void test_removeMultipleParenthesis(void)
{
	const char *eqColumn0 = "a";
	const char *eqColumn1 = "b";
	const char *inColumn  = "c";
	string statement =
	   StringUtils::sprintf("((%s=%s)) AND (((%s IN ('1','3'))))",
	                        eqColumn0, eqColumn1, inColumn);
	DEFINE_PARSER_AND_RUN(whereParser, formula, statement);
	formula->removeParenthesis();

	assertFormulaOperatorAnd(formula);
	FormulaElement *leftHand = formula->getLeftHand();
	assertFormulaComparatorEqual(leftHand);
	assertFormulaVariable(leftHand->getLeftHand(), eqColumn0);
	assertFormulaVariable(leftHand->getRightHand(), eqColumn1);

	assertTypeFormulaIn(formula->getRightHand());
}

} // namespace testSQLWhereParser

