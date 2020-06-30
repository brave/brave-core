/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_UNITTEST_UTILS_H_
#define BAT_CONFIRMATIONS_INTERNAL_UNITTEST_UTILS_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace confirmations {

class ConfirmationsClientMock;
class ConfirmationsImpl;

template<class T>
void Initialize(
    const T& object) {
  object->Initialize(
      [](const bool success) {
    ASSERT_TRUE(success);
  });
}

base::FilePath GetTestPath();

std::string GetPathForRequest(
    const std::string& url);

void MockLoadState(
    const std::unique_ptr<ConfirmationsClientMock>& mock);
void MockSaveState(
    const std::unique_ptr<ConfirmationsClientMock>& mock);

void MockClientInfo(
    const std::unique_ptr<ConfirmationsClientMock>& mock,
    const std::string& channel);

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_UNITTEST_UTILS_H_
