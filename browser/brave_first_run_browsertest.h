/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_FIRST_RUN_BROWSERTEST_H_
#define BRAVE_BROWSER_BRAVE_FIRST_RUN_BROWSERTEST_H_

// Copied from chrome/browser/first_run/first_run_browsertest.cc
// See browser/brave_profile_prefs_browsertest.cc for an example
// Useful for verifying first-run functionality.

#include <memory>
#include <string>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/metrics/histogram_tester.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/component_loader.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/first_run/first_run_internal.h"
#include "chrome/browser/importer/importer_list.h"
#include "chrome/browser/prefs/chrome_pref_service_factory.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "components/variations/metrics.h"
#include "components/variations/pref_names.h"
#include "components/variations/variations_switches.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_launcher.h"
#include "testing/gtest/include/gtest/gtest.h"

typedef InProcessBrowserTest FirstRunBrowserTest;

// A generic test class to be subclassed by test classes testing specific
// master_preferences. All subclasses must call SetMasterPreferencesForTest()
// from their SetUp() method before deferring the remainder of Setup() to this
// class.
class FirstRunMasterPrefsBrowserTestBase : public InProcessBrowserTest {
 public:
  FirstRunMasterPrefsBrowserTestBase();
  ~FirstRunMasterPrefsBrowserTestBase() override;

 protected:
  void SetUp() override;

  void TearDown() override;

  void SetUpCommandLine(base::CommandLine* command_line) override;

#if defined(OS_MAC) || defined(OS_LINUX)
  void SetUpInProcessBrowserTestFixture() override;
#endif

  void SetMasterPreferencesForTest(const char text[]) {
    text_.reset(new std::string(text));
  }

 private:
  base::FilePath prefs_file_;
  std::unique_ptr<std::string> text_;

  DISALLOW_COPY_AND_ASSIGN(FirstRunMasterPrefsBrowserTestBase);
};

template<const char Text[]>
class FirstRunMasterPrefsBrowserTestT
    : public FirstRunMasterPrefsBrowserTestBase {
 public:
  FirstRunMasterPrefsBrowserTestT() {}

 protected:
  void SetUp() override {
    SetMasterPreferencesForTest(Text);
    FirstRunMasterPrefsBrowserTestBase::SetUp();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(FirstRunMasterPrefsBrowserTestT);
};

#endif  // BRAVE_BROWSER_BRAVE_FIRST_RUN_BROWSERTEST_H_
