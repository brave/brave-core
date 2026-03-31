/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Brave adds background services (AI Chat, etc.) whose initialization triggers
// D-Bus operations on Linux. The D-Bus thread is a thread pool thread that
// posts replies back to the main thread via PostTaskAndReply. The upstream
// TimeRanges test calls ThreadPoolInstance::FlushForTesting() which blocks the
// main thread waiting for all thread pool tasks. This deadlocks because D-Bus
// tasks cannot complete until their reply callbacks run on the blocked main
// thread.
//
// Fix: Replace FlushForTesting() with content::RunAllTasksUntilIdle() which
// uses FlushAsyncForTesting() + RunLoop to pump the main thread message loop
// while waiting for thread pool tasks to complete, avoiding the deadlock.
// https://github.com/brave/brave-browser/issues/54038

#include "content/public/test/test_utils.h"

// Disable the original TimeRanges test; we provide a fixed version below.
#define TimeRanges DISABLED_TimeRanges_Upstream

#include <chrome/browser/browsing_data/counters/autofill_counter_browsertest.cc>

#undef TimeRanges

namespace {

IN_PROC_BROWSER_TEST_F(AutofillCounterTest, TimeRanges) {
  autofill::TestAutofillClock test_clock;
  const base::Time kTime1 = base::Time::FromSecondsSinceUnixEpoch(25);
  test_clock.SetNow(kTime1);
  AddAutocompleteSuggestion("email", "example@example.com");
  AddCreditCard("0000-0000-0000-0000", "1", "2015", "1");
  AddAddress("John", "Doe", "Main Street 12345");
  content::RunAllTasksUntilIdle();

  const base::Time kTime2 = kTime1 + base::Seconds(10);
  test_clock.SetNow(kTime2);
  AddCreditCard("0123-4567-8910-1112", "10", "2015", "1");
  AddAddress("Jane", "Smith", "Main Street 12346");
  AddAddress("John", "Smith", "Side Street 47");
  content::RunAllTasksUntilIdle();

  const base::Time kTime3 = kTime2 + base::Seconds(10);
  test_clock.SetNow(kTime3);
  AddAutocompleteSuggestion("tel", "+987654321");
  AddCreditCard("1211-1098-7654-3210", "10", "2030", "1");
  content::RunAllTasksUntilIdle();

  // Test the results for different starting points.
  struct TestCase {
    const base::Time period_start;
    const browsing_data::BrowsingDataCounter::ResultInt
        expected_num_suggestions;
    const browsing_data::BrowsingDataCounter::ResultInt
        expected_num_credit_cards;
    const browsing_data::BrowsingDataCounter::ResultInt expected_num_addresses;
  };
  auto test_cases = std::to_array<TestCase>({
      {base::Time(), 2, 3, 3},
      {kTime1, 2, 3, 3},
      {kTime2, 1, 2, 2},
      {kTime3, 1, 1, 0},
  });

  Profile* profile = browser()->profile();
  browsing_data::AutofillCounter counter = GetCounter();

  counter.Init(profile->GetPrefs(),
               browsing_data::ClearBrowsingDataTab::ADVANCED,
               future.GetRepeatingCallback());

  for (size_t i = 0; i < std::size(test_cases); i++) {
    SCOPED_TRACE(base::StringPrintf("Test case %zu", i));
    const auto& test_case = test_cases[i];
    counter.SetPeriodStartForTesting(test_case.period_start);
    counter.Restart();

    WaitForResult();

    EXPECT_EQ(test_case.expected_num_suggestions, GetCounterValue());
    EXPECT_EQ(test_case.expected_num_credit_cards, GetNumCreditCards());
    EXPECT_EQ(test_case.expected_num_addresses, GetNumAddresses());
    future.Clear();
  }

  // Test the results for different ending points and base::Time as start.
  counter.SetPeriodStartForTesting(base::Time());
  counter.SetPeriodEndForTesting(kTime2);
  counter.Restart();

  WaitForResult();
  EXPECT_EQ(1, GetCounterValue());
  EXPECT_EQ(1, GetNumCreditCards());
  EXPECT_EQ(1, GetNumAddresses());

  counter.SetPeriodEndForTesting(kTime3);
  counter.Restart();

  WaitForResult();

  EXPECT_EQ(1, GetCounterValue());
  EXPECT_EQ(2, GetNumCreditCards());
  EXPECT_EQ(3, GetNumAddresses());
}

}  // namespace
