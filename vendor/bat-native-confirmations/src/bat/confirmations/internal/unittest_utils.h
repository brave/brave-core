/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_UNITTEST_UTILS_H_
#define BAT_CONFIRMATIONS_INTERNAL_UNITTEST_UTILS_H_

#include <string>

#include "base/files/file_path.h"

namespace confirmations {

class ConfirmationsImpl;
class ConfirmationsClientMock;
class PlatformHelperMock;

void Initialize(
    ConfirmationsImpl* mock);

base::FilePath GetTestDataPath();

std::string GetPathForRequest(
    const std::string& url);

void MockLoadState(
    ConfirmationsClientMock* mock);
void MockSaveState(
    ConfirmationsClientMock* mock);

void MockClientInfo(
    ConfirmationsClientMock* mock,
    const std::string& channel);

void MockPlatformHelper(
    PlatformHelperMock* mock,
    const std::string& platform);

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_UNITTEST_UTILS_H_
