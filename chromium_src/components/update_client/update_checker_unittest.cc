/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/update_client/update_checker.h"

#include <memory>

#include "base/functional/bind.h"
#include "testing/gtest/include/gtest/gtest.h"

#include <components/update_client/update_checker_unittest.cc>

namespace update_client {

class UpdateCheckerTest;

// `UpdateEngine` stores the checker in `UpdateContext::update_checker`. Our
// `SequentialUpdateChecker` holds a reference back to the context. Once the
// check is done, that reference must be dropped, or the context (and
// everything it holds) leaks. In Omaha 4, this leak kept the updater's COM
// server alive indefinitely.
TEST_P(UpdateCheckerTest, SequentialUpdateCheckerReleasesContextWhenDone) {
  EXPECT_TRUE(post_interceptor_->ExpectRequest(
      std::make_unique<PartialMatch>("updatecheck"),
      GetTestFilePath("updatecheck_reply_1.json")));

  update_context_->components[kUpdateItemId] = MakeComponent();
  update_context_->update_checker = SequentialUpdateChecker::Create(config_);
  update_context_->update_checker->CheckForUpdates(
      update_context_, {},
      base::BindOnce(&UpdateCheckerTest::UpdateCheckComplete,
                     base::Unretained(this)));
  RunThreads();
  ASSERT_EQ(0, error_);

  // Only this test should still reference the context.
  EXPECT_TRUE(update_context_->HasOneRef());
}

}  // namespace update_client
