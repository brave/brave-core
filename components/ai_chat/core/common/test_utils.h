/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_TEST_UTILS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_TEST_UTILS_H_

#include <optional>
#include <string>
#include <vector>

#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"
#include "brave/components/ai_chat/core/common/test_mojom_printers.h"
#include "mojo/public/cpp/bindings/equals_traits.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

// macros to use MojomEqVerbose
#define EXPECT_MOJOM_EQ(a, b) EXPECT_PRED_FORMAT2(MojomEqVerbose, a, b)
#define ASSERT_MOJOM_EQ(a, b) ASSERT_PRED_FORMAT2(MojomEqVerbose, a, b)
#define EXPECT_MOJOM_NE(a, b) EXPECT_FALSE(mojo::Equals(a, b))

// Helper template function to compare two Mojom objects for equality,
// deeply, using mojo::Equals with verbose output
// via PrintTo implementation.
template <typename T>
::testing::AssertionResult MojomEqVerbose(const char* lhs_expr,
                                          const char* rhs_expr,
                                          const T& lhs,
                                          const T& rhs) {
  // Deep compare every field
  if (mojo::Equals(lhs, rhs)) {
    return ::testing::AssertionSuccess();
  }

  // Print both objects if the assertion fails
  return ::testing::AssertionFailure()
         << "\n"
         << lhs_expr << " {\n"
         << ::testing::PrintToString(lhs) << "}\n\n"
         << rhs_expr << " {\n"
         << ::testing::PrintToString(rhs) << "}\n\n";
}

std::vector<mojom::UploadedFilePtr> CreateSampleUploadedFiles(
    size_t number,
    mojom::UploadedFileType type);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_TEST_UTILS_H_
