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

#include <cppcutter.h>
#include "TimeCounter.h"

using namespace mlpl;

namespace testTimeCounter {

void test_constructorTimespec(void)
{
	timespec ts;
	ts.tv_sec = 1379641056;
	ts.tv_nsec = 987654321;
	TimeCounter timeCnt(ts);

	double actual = timeCnt.getAsSec();
	int actualInt = (int)actual;
	cppcut_assert_equal((int)ts.tv_sec, actualInt);
	int  actualDecimalPartUsec = (actual - actualInt) * 1e6;
	cppcut_assert_equal((int)(ts.tv_nsec/1e3), actualDecimalPartUsec);
}

} // namespace testTimeCounter
