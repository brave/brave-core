/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/unittest_utils.h"

#include "base/base_paths.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "url/gurl.h"
#include "bat/confirmations/internal/confirmations_client_mock.h"
#include "bat/confirmations/internal/platform_helper_mock.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

namespace confirmations {

base::FilePath GetDataPath() {
  base::FilePath path;
  base::PathService::Get(base::DIR_SOURCE_ROOT, &path);
  path = path.AppendASCII("brave");
  path = path.AppendASCII("vendor");
  path = path.AppendASCII("bat-native-confirmations");
  path = path.AppendASCII("data");
  return path;
}

base::FilePath GetTestPath() {
  base::FilePath path = GetDataPath();
  path = path.AppendASCII("test");
  return path;
}

std::string GetPathForRequest(
    const std::string& url) {
  return GURL(url).PathForRequest();
}

void MockLoadState(
    const std::unique_ptr<ConfirmationsClientMock>& mock) {
  ON_CALL(*mock, LoadState(_, _))
      .WillByDefault(Invoke([](
          const std::string& name,
          LoadCallback callback) {
        base::FilePath path = GetTestPath();
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
    const std::unique_ptr<ConfirmationsClientMock>& mock) {
  ON_CALL(*mock, SaveState(_, _, _))
      .WillByDefault(Invoke([](
          const std::string& name,
          const std::string& value,
          ResultCallback callback) {
        callback(SUCCESS);
      }));
}

void MockClientInfo(
    const std::unique_ptr<ConfirmationsClientMock>& mock,
    const std::string& channel) {
  ON_CALL(*mock, GetClientInfo())
      .WillByDefault([channel]() {
        ledger::ClientInfoPtr client = ledger::ClientInfo::New();
        client->channel = channel;
        return client;
      });
}

}  // namespace confirmations
