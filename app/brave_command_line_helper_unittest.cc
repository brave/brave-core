/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/app/brave_command_line_helper.h"

#include <set>
#include <string>
#include <vector>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

int CountA(const base::CommandLine::StringVector& sv) {
  int count = 0;
  for (const auto& argv : sv) {
    std::string value;
#if defined(OS_WIN)
    value = base::WideToUTF8(argv);
#else
    value = argv;
#endif
    if (value == "--a")
      count++;
  }
  return count;
}

std::set<std::string> FeaturesToSet(const std::string& features) {
  std::set<std::string> result;
  std::vector<std::string> values = base::SplitString(
      features, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  result.insert(std::make_move_iterator(values.begin()),
                std::make_move_iterator(values.end()));
  return result;
}

void CheckEnabledFeatures(const base::CommandLine& command_line,
                          const std::string& expected) {
  EXPECT_EQ(FeaturesToSet(
                command_line.GetSwitchValueASCII(switches::kEnableFeatures)),
            FeaturesToSet(expected));
}

void CheckDisabledFeatures(const base::CommandLine& command_line,
                           const std::string& expected) {
  EXPECT_EQ(FeaturesToSet(
                command_line.GetSwitchValueASCII(switches::kDisableFeatures)),
            FeaturesToSet(expected));
}

}  // namespace

TEST(BraveCommandLineHelperUnitTest, TestAppendSwitch) {
  base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();
  BraveCommandLineHelper helper(&command_line);
  // Test that append switch works.
  helper.AppendSwitch("a");
  ASSERT_TRUE(command_line.HasSwitch("a"));
  ASSERT_EQ(1, CountA(command_line.argv()));

  // Chromium's AppendSwtich always adds to argv even if the switch
  // has already been added.
  command_line.AppendSwitch("a");
  ASSERT_EQ(2, CountA(command_line.argv()));

  // Test that multiple attempts to append the same switch doesn't append
  // more than once.
  helper.AppendSwitch("a");
  ASSERT_EQ(2, CountA(command_line.argv()));
}

TEST(BraveCommandLineHelperUnitTest, TestParseFeatures) {
  base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();
  command_line.AppendSwitchASCII(switches::kEnableFeatures, "a,b,x");
  command_line.AppendSwitchASCII(switches::kDisableFeatures, "x,y,z");
  BraveCommandLineHelper helper(&command_line);
  // Test that intersecting enabled and disabled features have been removed from
  // enabled set.
  const std::unordered_set<std::string>& enabled = helper.enabled_features();
  ASSERT_EQ(std::set<std::string>(enabled.begin(), enabled.end()),
            FeaturesToSet("a,b"));
  const std::unordered_set<std::string>& disabled = helper.disabled_features();
  ASSERT_EQ(std::set<std::string>(disabled.begin(), disabled.end()),
            FeaturesToSet("x,y,z"));
}

TEST(BraveCommandLineHelperUnitTest, TestAppendFeatures) {
  base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();
  BraveCommandLineHelper helper(&command_line);
  // Test enabled features: none on command line.
  helper.AppendFeatures({"a"}, {});
  CheckEnabledFeatures(command_line, "a");
  CheckDisabledFeatures(command_line, "");

  // Test enabled features: merge with existing.
  helper.AppendFeatures({"a", "b", "c"}, {});
  CheckEnabledFeatures(command_line, "a,b,c");
  CheckDisabledFeatures(command_line, "");

  // Test disabled features: none on command line.
  helper.AppendFeatures({}, {"e", "f"});
  CheckEnabledFeatures(command_line, "a,b,c");
  CheckDisabledFeatures(command_line, "e,f");

  // Test disabled features: merge with existing.
  helper.AppendFeatures({}, {"e", "f", "g", "h"});
  CheckEnabledFeatures(command_line, "a,b,c");
  CheckDisabledFeatures(command_line, "e,f,g,h");

  // Test enabled features: intersects disabled.
  helper.AppendFeatures({"d", "x"}, {"x"});
  CheckEnabledFeatures(command_line, "a,b,c,d");
  CheckDisabledFeatures(command_line, "e,f,g,h,x");

  // Test disabled features: intersects with enabled.
  helper.AppendFeatures({}, {"c", "d", "i"});
  CheckEnabledFeatures(command_line, "a,b,c,d");
  CheckDisabledFeatures(command_line, "e,f,g,h,x,i");

  // Append same to enabled and disabled.
  helper.AppendFeatures({"v", "w"}, {"v", "w"});
  CheckEnabledFeatures(command_line, "a,b,c,d");
  CheckDisabledFeatures(command_line, "e,f,g,h,x,i,v,w");
}
