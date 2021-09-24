/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_importer_delegate_impl.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/browser/brave_wallet/keyring_controller_factory.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {
namespace {
const char* valid_mnemonic =
    "drip caution abandon festival order clown oven regular absorb evidence "
    "crew where";
// encrypted valid_mnemonic
const char* valid_data =
    "{\"data\": {\"KeyringController\": {\"vault\": "
    "\"{\\\"data\\\":"
    "\\\"CFJuPK8MgoieYbqCAc2aBQI4iToyI5KVwqkpMF6tKWkGt3r65pxFjwB2jylPkF0wrym4Or"
    "YGVY5UkapBVcAFwPSdN2TxPamTPwICT4G500lHnYQ+"
    "KLhCaNELEtaQ55tFvtrgE3SvedsB3QyHfqel6shrJtoZ1UWZbqttZGdjEG1gw8WHEJKYn5oav0"
    "G6rrSt9Gw+hvyQ1v4DWvPChNgaBh7qZpYCUneYuk59ixY5scEIEbdf9nr8fgM1Xf37fLQ="
    "\\\",\\\"iv\\\":\\\"fOHBjjQcsi1KmaeQ7xA7Aw==\\\", "
    "\\\"salt\\\":\\\"z1bTZtBY33d2l6CfiFs5V/eRQLS6Qsq5UtAQOIfaIps=\\\"}\"}}}";
}  // namespace

class BraveWalletImporterDelegateImplUnitTest : public testing::Test {
 public:
  BraveWalletImporterDelegateImplUnitTest() = default;

  void TearDown() override { importer_.reset(); }

  void SetUp() override {
    keyring_controller_ =
        KeyringControllerFactory::GetControllerForContext(browser_context());
    importer_ =
        std::make_unique<BraveWalletImporterDelegateImpl>(browser_context());
  }

  ~BraveWalletImporterDelegateImplUnitTest() override = default;

  KeyringController* keyring_controller() { return keyring_controller_; }
  BraveWalletImporterDelegateImpl* importer() { return importer_.get(); }

  content::BrowserContext* browser_context() { return &profile_; }

  void SimulateGetLocalStorage(const std::string& password,
                               const std::string& new_password,
                               const std::string& json_str,
                               bool* out_success,
                               bool* callback_is_called) {
    ASSERT_NE(out_success, nullptr);
    ASSERT_NE(callback_is_called, nullptr);
    auto json = base::JSONReader::Read(json_str);
    ASSERT_TRUE(json);
    importer_->OnGetLocalStorage(
        password, new_password, base::BindLambdaForTesting([&](bool success) {
          *out_success = success;
          *callback_is_called = true;
        }),
        base::DictionaryValue::From(
            base::Value::ToUniquePtrValue(std::move(*json))));
    base::RunLoop().RunUntilIdle();
  }

 protected:
  content::BrowserTaskEnvironment browser_task_environment_;

 private:
  KeyringController* keyring_controller_;
  std::unique_ptr<BraveWalletImporterDelegateImpl> importer_;
  TestingProfile profile_;
};

TEST_F(BraveWalletImporterDelegateImplUnitTest, OnGetLocalStorageError) {
  bool callback_is_called = false;
  bool result = true;
  // empty password
  SimulateGetLocalStorage("", "", valid_data, &result, &callback_is_called);
  EXPECT_TRUE(callback_is_called);
  EXPECT_FALSE(result);

  // TODO(darkdh): remove this when we support legacy 24 words mnemonic
  // decryption
  callback_is_called = false;
  result = true;
  SimulateGetLocalStorage(
      "123", "1234",
      R"({"data": { "KeyringController": { "argonParams": {} } }})", &result,
      &callback_is_called);
  EXPECT_TRUE(callback_is_called);
  EXPECT_FALSE(result);

  callback_is_called = false;
  result = true;
  // no vault
  SimulateGetLocalStorage("123", "1234",
                          R"({"data": { "KeyringController": {}}})", &result,
                          &callback_is_called);
  EXPECT_TRUE(callback_is_called);
  EXPECT_FALSE(result);

  callback_is_called = false;
  result = true;
  // vault is not a valid json
  SimulateGetLocalStorage(
      "123", "1234", R"({"data": { "KeyringController": { "vault": "{[}]"}}})",
      &result, &callback_is_called);
  EXPECT_TRUE(callback_is_called);
  EXPECT_FALSE(result);

  callback_is_called = false;
  result = true;
  // vault missing iv and salt
  SimulateGetLocalStorage(
      "123", "1234",
      R"({"data": { "KeyringController": { "vault": "{\"data\": \"data\"}"}}})",
      &result, &callback_is_called);
  EXPECT_TRUE(callback_is_called);
  EXPECT_FALSE(result);

  callback_is_called = false;
  result = true;
  // data is not base64 encoded
  SimulateGetLocalStorage("123", "1234",
                          R"({"data": {"KeyringController": {
                          "vault": "{\"data\": \"data\",
                          \"iv\": \"aXY=\", \"salt\": \"c2FsdA==\"}"}}})",
                          &result, &callback_is_called);
  EXPECT_TRUE(callback_is_called);
  EXPECT_FALSE(result);

  callback_is_called = false;
  result = true;
  // wrong password
  SimulateGetLocalStorage("123", "1234", valid_data, &result,
                          &callback_is_called);
  EXPECT_TRUE(callback_is_called);
  EXPECT_FALSE(result);
}

TEST_F(BraveWalletImporterDelegateImplUnitTest, OnGetLocalStorage) {
  bool callback_is_called = false;
  bool result = false;
  SimulateGetLocalStorage("brave4ever", "brave5ever", valid_data, &result,
                          &callback_is_called);
  EXPECT_TRUE(callback_is_called);
  EXPECT_TRUE(result);

  keyring_controller()->Lock();
  callback_is_called = false;
  // Check new password
  keyring_controller()->Unlock("brave5ever",
                               base::BindLambdaForTesting([&](bool success) {
                                 EXPECT_TRUE(success);
                                 callback_is_called = true;
                               }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_is_called);

  callback_is_called = false;
  keyring_controller()->GetMnemonicForDefaultKeyring(
      base::BindLambdaForTesting([&](const std::string& mnemonic) {
        EXPECT_EQ(mnemonic, valid_mnemonic);
        callback_is_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_is_called);
}

}  // namespace brave_wallet
