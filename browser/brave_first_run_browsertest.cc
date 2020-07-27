/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_first_run_browsertest.h"

FirstRunMasterPrefsBrowserTestBase::FirstRunMasterPrefsBrowserTestBase() {}

FirstRunMasterPrefsBrowserTestBase::~FirstRunMasterPrefsBrowserTestBase() {}

void FirstRunMasterPrefsBrowserTestBase::SetUp() {
  // All users of this test class need to call SetMasterPreferencesForTest()
  // before this class' SetUp() is invoked.
  ASSERT_TRUE(text_.get());

  ASSERT_TRUE(base::CreateTemporaryFile(&prefs_file_));
  EXPECT_EQ(static_cast<int>(text_->size()),
            base::WriteFile(prefs_file_, text_->c_str(), text_->size()));
  first_run::SetInitialPrefsPathForTesting(prefs_file_);

  // This invokes BrowserMain, and does the import, so must be done last.
  InProcessBrowserTest::SetUp();
}

void FirstRunMasterPrefsBrowserTestBase::TearDown() {
  EXPECT_TRUE(base::DeleteFile(prefs_file_, false));
  InProcessBrowserTest::TearDown();
}

void FirstRunMasterPrefsBrowserTestBase::SetUpCommandLine(
    base::CommandLine* command_line) {
  command_line->AppendSwitch(switches::kForceFirstRun);
  EXPECT_EQ(first_run::AUTO_IMPORT_NONE, first_run::auto_import_state());

  extensions::ComponentLoader::EnableBackgroundExtensionsForTesting();
}

#if defined(OS_MACOSX) || defined(OS_LINUX)
void FirstRunMasterPrefsBrowserTestBase::SetUpInProcessBrowserTestFixture() {
  InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
  // Suppress first run dialog since it blocks test progress.
  first_run::internal::ForceFirstRunDialogShownForTesting(false);
}
#endif
