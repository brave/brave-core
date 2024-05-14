/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/external_wallets_importer.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/test/bind.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {
namespace {
// encrypted valid_mnemonic with legacy 10K iterations for pbkdf2
const char* valid_data_10K =
    "{\"data\": {\"KeyringController\": {\"vault\": "
    "\"{\\\"data\\\":"
    "\\\"CFJuPK8MgoieYbqCAc2aBQI4iToyI5KVwqkpMF6tKWkGt3r65pxFjwB2jylPkF0wrym4Or"
    "YGVY5UkapBVcAFwPSdN2TxPamTPwICT4G500lHnYQ+"
    "KLhCaNELEtaQ55tFvtrgE3SvedsB3QyHfqel6shrJtoZ1UWZbqttZGdjEG1gw8WHEJKYn5oav0"
    "G6rrSt9Gw+hvyQ1v4DWvPChNgaBh7qZpYCUneYuk59ixY5scEIEbdf9nr8fgM1Xf37fLQ="
    "\\\",\\\"iv\\\":\\\"fOHBjjQcsi1KmaeQ7xA7Aw==\\\", "
    "\\\"salt\\\":\\\"z1bTZtBY33d2l6CfiFs5V/eRQLS6Qsq5UtAQOIfaIps=\\\"}\"}}}";

// encrypted mnemonic with 600K iterations for pbkdf2
const char* valid_data_600K =
    "{\"data\": {\"KeyringController\": {\"vault\": "
    "\"{\\\"data\\\":"
    "\\\"Ch/424qkkgfpp9IQelclwYEylFdVwlaBPq2qZUItr5iPm+"
    "bjzVN7QGViGyPXDnyHmMLIa1IWDsNAN0qibd4xAtD+"
    "uTvgYLRpzg1tVEezqxLub7l2iF6GuNJoQSexHgDrSEmduoFv27Exw6oSPZ/"
    "3pHKPHFWhD8b9RIpYdQHDNex4m39Dkim9fuMSretUe3xt7ZUe76bA6wgfhgPBqSXmj5KOx5gNH"
    "uZ69qH+IJbjGhylkUN8BSbJMDjRz7JOhvXgZN34iTB8fNQhweLGTLCF0VVyaUObp9egF/"
    "TGMTrQZFunqoSKsmywvDxI55REmcL8PLiszHI9Zbj5Vcan5GSRc2oKX3MGYBNG0oqqVVSYooaa"
    "1jqLHeuc4f/RsAPDvKr8LQzGw/MHpZ51W2IClU6hk4CjkiBjIG/9TS6RQCQlJkf5Cd22meQH/"
    "nrP3H+t3mrmqBGnkneP6+7Ne84+QZ+ysuhhy/"
    "MQV3eST7lOreKwmX2mZwPBQz0WJBoIX6koGNGgcM6sdq6y/"
    "Dl6V2XTbfom0GzLuDOcaxmy3w==\\\",\\\"iv\\\":\\\"UxlJX/"
    "Bi0ur1E+KvUhXnUA==\\\",\\\"salt\\\":"
    "\\\"ha59QKTWKPDqx83ZbPnCra3SFt37uLuZxF9nQamI3BM=\\\"}\"}}}";

const char* valid_data_with_utf8_mnemonic =
    "{\"data\": {\"KeyringController\": {\"vault\": "
    "\"{\\\"data\\\":\\\"Q27H35GCkppku8PtVmiPsNJNfe5wjSWgjD5JGa3jtTlmwaTBffWJL+"
    "cadVr5X8c0JnPToVUwbJXcIdmKxT8vAWZWQqRoeiTXOl6iF9SaoqhhnOIX1+"
    "FPEPyHV4bu3GUpUVokgdYA1eryw39sQxFm5gLyl44VfF8hmuG+"
    "2c4nEmPbK7XBDMSwif4Q1jas4CkhHBKGL3j6x7jpyMBtku4FK5LpFC5+G+A/"
    "OOOUPFQWUpct5JidweZWFoABHz0WIRGnZXWeFE/BoO+/"
    "JaHN08k9jQH4TMw6TylVODgqxVk1EqsYOvJfVZIRIjP7no0c94ZlyukcUOmtuFWE2N4swndqUB"
    "TPBobISrSyBIK/SbgMJRcK/VwYlXRAjDCKJ8WIhVezPm8pZap2e6SM/"
    "cKs0ScKe7Ngjw25UHKRB1QAoVgCbeJiv+"
    "UqpuGpcFAbrZ1tYcJyqJkguw8fMMWiehtmYubzFx4plXzcz7h4ZHbnkzR7BNHUCemmFhsXxTpe"
    "UtvH3kcDKtSu4H0JwUMMh7a8gCp/MYZxMxGo2aSKKLBkpW0l/mt/"
    "IWgChfXq1h7Ch3hCxGdG+mNx/mZ8xkXakzJzPw20MNdejx5gqF/pUp/"
    "jRGbSaPCaVhkT2a0rXnj8YFjMJbGuPnOn8hmSanIOOK1ETwkQolA+"
    "jo8qyNXFtmsCmyrbdSPfEFLZGC0MyUD4viNN2aRoIDa8339YF4C8qkg3U0Zh6z0gmbgnNDMAjn"
    "BmFl5sCGtRolu9pT+EJAE9XGDh5cvSCA7YMeLQTvLrhDn8o8kXc8J92yjw\\\",\\\"iv\\\":"
    "\\\"mmSwsbEsytQDfdNBP6WwOw==\\\",\\\"salt\\\":"
    "\\\"ZNDNQqgIaLswCtSH72AwaaymPQqmO6VCgpbfAmAuw5s=\\\"}\"}}}";

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

class ExternalWalletsImporterUnitTest : public testing::Test {
 public:
  ExternalWalletsImporterUnitTest() = default;

  ~ExternalWalletsImporterUnitTest() override = default;

  content::BrowserContext* browser_context() { return &profile_; }

  void SimulateGetImportInfo(const std::string& password,
                             const std::string& json_str,
                             bool* out_success,
                             ImportInfo* out_info,
                             ImportError* out_error) {
    ASSERT_NE(out_success, nullptr);

    auto json =
        base::JSONReader::Read(json_str, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                             base::JSON_ALLOW_CONTROL_CHARS);
    ASSERT_TRUE(json);
    {
      ExternalWalletsImporter importer(mojom::ExternalWalletType::CryptoWallets,
                                       browser_context());

      importer.SetStorageDataForTesting(std::move(*json->Clone().GetIfDict()));

      base::RunLoop run_loop;
      importer.GetImportInfo(
          password, base::BindLambdaForTesting(
                        [&](bool success, ImportInfo info, ImportError error) {
                          *out_success = success;
                          *out_info = info;
                          *out_error = error;
                          run_loop.Quit();
                        }));
      run_loop.Run();
    }
    {
      ExternalWalletsImporter importer(mojom::ExternalWalletType::MetaMask,
                                       browser_context());

      importer.SetStorageDataForTesting(std::move(*json->GetIfDict()));

      base::RunLoop run_loop;
      importer.GetImportInfo(
          password, base::BindLambdaForTesting(
                        [&](bool success, ImportInfo info, ImportError error) {
                          EXPECT_EQ(*out_success, success);
                          EXPECT_EQ(out_info->mnemonic, info.mnemonic);
                          EXPECT_EQ(out_info->is_legacy_crypto_wallets,
                                    info.is_legacy_crypto_wallets);
                          EXPECT_EQ(out_info->number_of_accounts,
                                    info.number_of_accounts);
                          EXPECT_EQ(*out_error, error);
                          run_loop.Quit();
                        }));
      run_loop.Run();
    }
  }

  void SimulateIsExternalWalletInitialized(const std::string& json_str,
                                           bool* out_initialized) {
    ASSERT_NE(out_initialized, nullptr);
    ExternalWalletsImporter cw_importer(
        mojom::ExternalWalletType::CryptoWallets, browser_context());
    ExternalWalletsImporter mm_importer(mojom::ExternalWalletType::MetaMask,
                                        browser_context());
    auto json = base::JSONReader::Read(json_str);
    ASSERT_TRUE(json);
    cw_importer.SetStorageDataForTesting(std::move(*json->Clone().GetIfDict()));
    mm_importer.SetStorageDataForTesting(std::move(*json->GetIfDict()));
    cw_importer.SetExternalWalletInstalledForTesting(true);
    mm_importer.SetExternalWalletInstalledForTesting(true);
    *out_initialized = cw_importer.IsExternalWalletInitialized();
    EXPECT_EQ(mm_importer.IsExternalWalletInitialized(), *out_initialized);
  }

 protected:
  content::BrowserTaskEnvironment browser_task_environment_;

 private:
  TestingProfile profile_;
};

TEST_F(ExternalWalletsImporterUnitTest, OnGetImportInfoError) {
  bool result = true;
  ImportInfo info;
  ImportError error;
  // empty password
  SimulateGetImportInfo("", valid_data_10K, &result, &info, &error);
  EXPECT_FALSE(result);
  EXPECT_EQ(error, ImportError::kPasswordError);

  result = true;
  error = ImportError::kNone;
  // no vault
  SimulateGetImportInfo("123", R"({"data": { "KeyringController": {}}})",
                        &result, &info, &error);
  EXPECT_FALSE(result);
  EXPECT_EQ(error, ImportError::kJsonError);

  result = true;
  error = ImportError::kNone;
  // vault is not a valid json
  SimulateGetImportInfo(
      "123", R"({"data": { "KeyringController": { "vault": "{[}]"}}})", &result,
      &info, &error);
  EXPECT_FALSE(result);
  EXPECT_EQ(error, ImportError::kJsonError);

  result = true;
  error = ImportError::kNone;
  // vault missing iv and salt
  SimulateGetImportInfo(
      "123",
      R"({"data": { "KeyringController": { "vault": "{\"data\": \"data\"}"}}})",
      &result, &info, &error);
  EXPECT_FALSE(result);
  EXPECT_EQ(error, ImportError::kJsonError);

  result = true;
  error = ImportError::kNone;
  // data is not base64 encoded
  SimulateGetImportInfo("123",
                        R"({"data": {"KeyringController": {
                          "vault": "{\"data\": \"d\",
                          \"iv\": \"aXY=\", \"salt\": \"c2FsdA==\"}"}}})",
                        &result, &info, &error);
  EXPECT_FALSE(result);
  EXPECT_EQ(error, ImportError::kJsonError);

  result = true;
  error = ImportError::kNone;
  // wrong password
  SimulateGetImportInfo("123", valid_data_10K, &result, &info, &error);
  EXPECT_FALSE(result);
  EXPECT_EQ(error, ImportError::kPasswordError);
}

TEST_F(ExternalWalletsImporterUnitTest, OnGetImportInfo_10K_Iterations) {
  bool result = false;
  ImportInfo info;
  ImportError error;
  SimulateGetImportInfo("brave4ever", valid_data_10K, &result, &info, &error);
  EXPECT_TRUE(result);
  EXPECT_EQ(error, ImportError::kNone);
  EXPECT_EQ(info.mnemonic, kMnemonicDripCaution);
  EXPECT_FALSE(info.is_legacy_crypto_wallets);
  EXPECT_EQ(info.number_of_accounts, 1u);
}

TEST_F(ExternalWalletsImporterUnitTest, OnGetImportInfo_600K_Iterations) {
  bool result = false;
  ImportInfo info;
  ImportError error;
  SimulateGetImportInfo("12345qwert", valid_data_600K, &result, &info, &error);
  EXPECT_TRUE(result);
  EXPECT_EQ(error, ImportError::kNone);
  EXPECT_EQ(info.mnemonic,
            "try fossil lesson direct toddler favorite wedding opera camera "
            "sand great hammer");
  EXPECT_FALSE(info.is_legacy_crypto_wallets);
  EXPECT_EQ(info.number_of_accounts, 1u);
}

TEST_F(ExternalWalletsImporterUnitTest, OnGetImportInfo_UTF8Mnemonic) {
  bool result = false;
  ImportInfo info;
  ImportError error;
  SimulateGetImportInfo("brave4ever", valid_data_with_utf8_mnemonic, &result,
                        &info, &error);
  EXPECT_TRUE(result);
  EXPECT_EQ(error, ImportError::kNone);
  EXPECT_EQ(info.mnemonic, kMnemonicDripCaution);
  EXPECT_FALSE(info.is_legacy_crypto_wallets);
  EXPECT_EQ(info.number_of_accounts, 1u);
}

TEST_F(ExternalWalletsImporterUnitTest, ImportLegacyWalletError) {
  bool result = true;
  // argonParams is not a dict
  ImportInfo info;
  ImportError error;
  SimulateGetImportInfo("123", R"({
          "data": { "KeyringController": {
                  "argonParams": "123"
              }}})",
                        &result, &info, &error);
  EXPECT_FALSE(result);
  EXPECT_EQ(error, ImportError::kInternalError);

  result = true;
  error = ImportError::kNone;
  // argonParams multiple fields are missing
  SimulateGetImportInfo("123", R"({
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
  SimulateGetImportInfo("123", R"({
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
  SimulateGetImportInfo("123", R"({
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

TEST_F(ExternalWalletsImporterUnitTest, ImportLegacyWallet) {
  bool result = false;
  ImportInfo info;
  ImportError error;
  SimulateGetImportInfo("bbbravey", valid_legacy_data, &result, &info, &error);
  EXPECT_TRUE(result);
  EXPECT_EQ(error, ImportError::kNone);
  EXPECT_EQ(info.mnemonic, valid_legacy_mnemonic);
  EXPECT_TRUE(info.is_legacy_crypto_wallets);
  EXPECT_EQ(info.number_of_accounts, 2u);
}

TEST_F(ExternalWalletsImporterUnitTest, IsExternalWalletInitialized) {
  bool initialized = false;
  SimulateIsExternalWalletInitialized("{\"data\":{\"KeyringController\":{}}}",
                                      &initialized);
  EXPECT_TRUE(initialized);

  initialized = true;
  SimulateIsExternalWalletInitialized(
      "{\"data\":{\"KeyringProController\":{}}}", &initialized);
  EXPECT_FALSE(initialized);

  initialized = true;
  SimulateIsExternalWalletInitialized("{\"KeyringController\":{}}",
                                      &initialized);
  EXPECT_FALSE(initialized);
}

}  // namespace brave_wallet
