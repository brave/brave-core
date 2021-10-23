/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_service_delegate_impl.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
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

const char* valid_legacy_mnemonic =
    "cushion pitch impact album daring marine much annual budget social "
    "clarify "
    "balance rose almost area busy among bring hidden bind later capable pulp "
    "laundry";
const char* valid_legacy_data =
    "{\"data\": {\"KeyringController\": {"
    "\"argonParams\": {"
    "\"hashLen\": 32,"
    "\"mem\": 500000,"
    "\"time\": 1,"
    "\"type\": 2"
    "},"
    "\"salt\": \"�t\u0003c͓��:BX��R��VE�N��[�[���h�\","  // NOLINT
    "\"vault\": "
    "\"{\\\"data\\\":\\\"z4NZSfTYHg2DBDqlkXYa5rmB4LwtL9pw5MKY3RhBYPh6qHgYO/YwO/"
    "jkX6Xdie6vtqbyo2v/juXopeuGOVWv29z8uBlOdKtHgZWhmG0hjnjemEd//"
    "vhxf57CR7GLTV25l0mxFM4ZAh8D8lrf5A8h1G517XvF+Nw+hyuiPYKKrezujrBfr0BxhN0nq+"
    "y5Yfehcge1SPpIZO+KTY2SDFkYBuv4EixHRNYAPTP/"
    "HiLvGXIectog1E5SoykqaLcbxIDDXzDBGm1psvLRuLj1fRGIp+vi7T2B5QUTnk/"
    "mJuzxMbxB5EQICDaGYkA+TikvnalHiDQ5N2UE+EgxoJJvf4Hbrn88CEd/"
    "RTAxRA==\\\",\\\"iv\\\":\\\"F+H7Yn5bDI5tgMmtpy5Wlg==\\\",\\\"salt\\\":"
    "\\\"p7eG29poyGVjP4aeaN175BV0g+SaFKGtyhLHEkLbuyg=\\\"}\"}}}";

}  // namespace

class BraveWalletServiceDelegateImplUnitTest : public testing::Test {
 public:
  BraveWalletServiceDelegateImplUnitTest() = default;

  using ImportInfo = BraveWalletServiceDelegate::ImportInfo;
  using ImportError = BraveWalletServiceDelegate::ImportError;

  void TearDown() override { delegate_.reset(); }

  void SetUp() override {
    delegate_ =
        std::make_unique<BraveWalletServiceDelegateImpl>(browser_context());
  }

  ~BraveWalletServiceDelegateImplUnitTest() override = default;

  content::BrowserContext* browser_context() { return &profile_; }

  void SimulateGetLocalStorage(const std::string& password,
                               const std::string& json_str,
                               bool* out_success,
                               ImportInfo* out_info,
                               ImportError* out_error) {
    ASSERT_NE(out_success, nullptr);

    auto json = base::JSONReader::Read(json_str);
    ASSERT_TRUE(json);

    base::RunLoop run_loop;
    delegate_->OnGetLocalStorage(
        password,
        base::BindLambdaForTesting(
            [&](bool success, ImportInfo info, ImportError error) {
              *out_success = success;
              *out_info = info;
              *out_error = error;
              run_loop.Quit();
            }),
        base::DictionaryValue::From(
            base::Value::ToUniquePtrValue(std::move(*json))));
    run_loop.Run();
  }

 protected:
  content::BrowserTaskEnvironment browser_task_environment_;

 private:
  std::unique_ptr<BraveWalletServiceDelegateImpl> delegate_;
  TestingProfile profile_;
};

TEST_F(BraveWalletServiceDelegateImplUnitTest, OnGetLocalStorageError) {
  bool result = true;
  ImportInfo info;
  ImportError error;
  // empty password
  SimulateGetLocalStorage("", valid_data, &result, &info, &error);
  EXPECT_FALSE(result);
  EXPECT_EQ(error, ImportError::kPasswordError);

  result = true;
  error = ImportError::kNone;
  // no vault
  SimulateGetLocalStorage("123", R"({"data": { "KeyringController": {}}})",
                          &result, &info, &error);
  EXPECT_FALSE(result);
  EXPECT_EQ(error, ImportError::kJsonError);

  result = true;
  error = ImportError::kNone;
  // vault is not a valid json
  SimulateGetLocalStorage(
      "123", R"({"data": { "KeyringController": { "vault": "{[}]"}}})", &result,
      &info, &error);
  EXPECT_FALSE(result);
  EXPECT_EQ(error, ImportError::kJsonError);

  result = true;
  error = ImportError::kNone;
  // vault missing iv and salt
  SimulateGetLocalStorage(
      "123",
      R"({"data": { "KeyringController": { "vault": "{\"data\": \"data\"}"}}})",
      &result, &info, &error);
  EXPECT_FALSE(result);
  EXPECT_EQ(error, ImportError::kJsonError);

  result = true;
  error = ImportError::kNone;
  // data is not base64 encoded
  SimulateGetLocalStorage("123",
                          R"({"data": {"KeyringController": {
                          "vault": "{\"data\": \"d\",
                          \"iv\": \"aXY=\", \"salt\": \"c2FsdA==\"}"}}})",
                          &result, &info, &error);
  EXPECT_FALSE(result);
  EXPECT_EQ(error, ImportError::kJsonError);

  result = true;
  error = ImportError::kNone;
  // wrong password
  SimulateGetLocalStorage("123", valid_data, &result, &info, &error);
  EXPECT_FALSE(result);
  EXPECT_EQ(error, ImportError::kPasswordError);
}

TEST_F(BraveWalletServiceDelegateImplUnitTest, OnGetLocalStorage) {
  bool result = false;
  ImportInfo info;
  ImportError error;
  SimulateGetLocalStorage("brave4ever", valid_data, &result, &info, &error);
  EXPECT_TRUE(result);
  EXPECT_EQ(error, ImportError::kNone);
  EXPECT_EQ(info.mnemonic, valid_mnemonic);
  EXPECT_FALSE(info.is_legacy_crypto_wallets);
  EXPECT_EQ(info.number_of_accounts, 1u);
}

TEST_F(BraveWalletServiceDelegateImplUnitTest, ImportLegacyWalletError) {
  bool result = true;
  // argonParams is not a dict
  ImportInfo info;
  ImportError error;
  SimulateGetLocalStorage("123", R"({
          "data": { "KeyringController": {
                  "argonParams": "123"
              }}})",
                          &result, &info, &error);
  EXPECT_FALSE(result);
  EXPECT_EQ(error, ImportError::kInternalError);

  result = true;
  error = ImportError::kNone;
  // argonParams multiple fields are missing
  SimulateGetLocalStorage("123", R"({
          "data": { "KeyringController": {
                  "argonParams": {
                    "mem": 256
                  }
              }}})",
                          &result, &info, &error);
  EXPECT_FALSE(result);
  EXPECT_EQ(error, ImportError::kInternalError);

  result = true;
  error = ImportError::kNone;
  // argonParams type is not 2
  SimulateGetLocalStorage("123", R"({
          "data": { "KeyringController": {
                  "argonParams": {
                    "hashLen": 32,
                    "mem": 500000,
                    "time": 1,
                    "type": 1
                  }
              }}})",
                          &result, &info, &error);
  EXPECT_FALSE(result);
  EXPECT_EQ(error, ImportError::kInternalError);

  result = true;
  error = ImportError::kNone;
  // KeyringController.salt is missing
  SimulateGetLocalStorage("123", R"({
          "data": { "KeyringController": {
                  "argonParams": {
                    "hashLen": 32,
                    "mem": 500000,
                    "time": 1,
                    "type": 2
                  }
              }}})",
                          &result, &info, &error);
  EXPECT_FALSE(result);
  EXPECT_EQ(error, ImportError::kInternalError);
}

TEST_F(BraveWalletServiceDelegateImplUnitTest, ImportLegacyWallet) {
  bool result = false;
  ImportInfo info;
  ImportError error;
  SimulateGetLocalStorage("bbbravey", valid_legacy_data, &result, &info,
                          &error);
  EXPECT_TRUE(result);
  EXPECT_EQ(error, ImportError::kNone);
  EXPECT_EQ(info.mnemonic, valid_legacy_mnemonic);
  EXPECT_TRUE(info.is_legacy_crypto_wallets);
  EXPECT_EQ(info.number_of_accounts, 2u);
}

}  // namespace brave_wallet
