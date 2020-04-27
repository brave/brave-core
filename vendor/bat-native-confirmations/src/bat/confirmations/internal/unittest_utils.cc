/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/unittest_utils.h"

#include "bat/confirmations/internal/confirmations_client_mock.h"
#include "bat/confirmations/internal/confirmations_impl_mock.h"
#include "bat/confirmations/internal/platform_helper_mock.h"

#include "base/files/file_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "url/gurl.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

namespace confirmations {

void Initialize(
    ConfirmationsImpl* confirmations) {
  confirmations->Initialize(
      [](const bool success) {
    ASSERT_TRUE(success);
  });
}

base::FilePath GetTestDataPath() {
  return base::FilePath("brave/vendor/bat-native-confirmations/test/data");
}

std::string GetPathForRequest(
    const std::string& url) {
  return GURL(url).PathForRequest();
}

void MockLoadState(
    ConfirmationsClientMock* mock) {
  ON_CALL(*mock, LoadState(_, _))
      .WillByDefault(Invoke([](
          const std::string& name,
          LoadCallback callback) {
        base::FilePath path = GetTestDataPath();
        path = path.AppendASCII(name);

        std::string value;
        if (!base::ReadFileToString(path, &value)) {
          callback(FAILED, value);
          return;
        }

        callback(SUCCESS, value);
      }));
}

void MockSaveState(
    ConfirmationsClientMock* mock) {
  ON_CALL(*mock, SaveState(_, _, _))
      .WillByDefault(Invoke([](
          const std::string& name,
          const std::string& value,
          ResultCallback callback) {
        callback(SUCCESS);
      }));
}

void MockClientInfo(
    ConfirmationsClientMock* mock,
    const std::string& channel) {
  ON_CALL(*mock, GetClientInfo())
      .WillByDefault([channel]() {
        ledger::ClientInfoPtr client = ledger::ClientInfo::New();
        client->channel = channel;
        return client;
      });
}

void MockPlatformHelper(
    PlatformHelperMock* mock,
    const std::string& platform) {
  PlatformHelper::GetInstance()->set_for_testing(mock);

  ON_CALL(*mock, GetPlatformName())
      .WillByDefault(Return(platform));
}

}  // namespace confirmations
