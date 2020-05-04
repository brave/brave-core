/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads_unittest_utils.h"

#include <string>
#include <vector>

#include "bat/ads/internal/ads_client_mock.h"

#include "base/base_paths.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "testing/gmock/include/gmock/gmock.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;

namespace ads {

base::FilePath GetDataPath() {
  base::FilePath path;
  base::PathService::Get(base::DIR_SOURCE_ROOT, &path);
  path = path.AppendASCII("brave");
  path = path.AppendASCII("vendor");
  path = path.AppendASCII("bat-native-ads");
  path = path.AppendASCII("data");
  return path;
}

base::FilePath GetTestPath() {
  base::FilePath path = GetDataPath();
  path = path.AppendASCII("test");
  return path;
}

base::FilePath GetResourcesPath() {
  base::FilePath path = GetDataPath();
  path = path.AppendASCII("resources");
  return path;
}

void MockLoad(
    AdsClientMock* mock) {
  ON_CALL(*mock, Load(_, _))
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

void MockSave(
    AdsClientMock* mock) {
  ON_CALL(*mock, Save(_, _, _))
      .WillByDefault(Invoke([](
          const std::string& name,
          const std::string& value,
          ResultCallback callback) {
        callback(SUCCESS);
      }));
}

void MockLoadUserModelForLanguage(
    AdsClientMock* mock) {
  const std::vector<std::string> user_model_languages = { "en", "de", "fr" };
  EXPECT_CALL(*mock, GetUserModelLanguages())
      .WillRepeatedly(Return(user_model_languages));

  EXPECT_CALL(*mock, LoadUserModelForLanguage(_, _))
      .WillRepeatedly(Invoke([](
          const std::string& language,
          LoadCallback callback) {
        base::FilePath path = GetResourcesPath();
        path = path.AppendASCII("user_models");
        path = path.AppendASCII("languages");
        path = path.AppendASCII(language);
        path = path.AppendASCII("user_model.json");

        std::string value;
        if (!base::ReadFileToString(path, &value)) {
          callback(FAILED, value);
          return;
        }

        callback(SUCCESS, value);
      }));
}

void MockLoadJsonSchema(
    AdsClientMock* mock) {
  EXPECT_CALL(*mock, LoadJsonSchema(_))
      .WillRepeatedly(Invoke([](
          const std::string& name) -> std::string {
        base::FilePath path = GetTestPath();
        path = path.AppendASCII(name);

        std::string value;
        base::ReadFileToString(path, &value);

        return value;
      }));
}

}  // namespace ads
