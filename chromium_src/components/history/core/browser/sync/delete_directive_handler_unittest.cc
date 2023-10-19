/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/history/core/browser/sync/delete_directive_handler_unittest.cc"

namespace history {
namespace {

TEST_F(HistoryDeleteDirectiveHandlerTest,
       BraveCreateUrlDeleteDirectiveReturnsFalse) {
  EXPECT_FALSE(handler()->CreateUrlDeleteDirective(GURL("https://brave.com")));
}

}  // namespace
}  // namespace history
