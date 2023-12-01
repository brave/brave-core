// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ipfs/pin/ipfs_local_pin_service.h"

#include <memory>
#include <optional>
#include <set>
#include <utility>

#include "base/json/json_reader.h"
#include "base/test/bind.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;

namespace ipfs {

namespace {

class MockIpfsService : public IpfsService {
 public:
  MockIpfsService() = default;

  ~MockIpfsService() override = default;

  MOCK_METHOD3(AddPin,
               void(const std::vector<std::string>& cids,
                    bool recursive,
                    IpfsService::AddPinCallback callback));
  MOCK_METHOD2(RemovePin,
               void(const std::vector<std::string>& cid,
                    IpfsService::RemovePinCallback callback));
  MOCK_METHOD4(GetPins,
               void(const std::optional<std::vector<std::string>>& cid,
                    const std::string& type,
                    bool quiet,
                    IpfsService::GetPinsCallback callback));
  MOCK_METHOD2(RemovePinCli,
               void(std::set<std::string> cid,
                    IpfsService::BoolCallback callback));
  MOCK_METHOD1(LsPinCli, void(IpfsService::NodeCallback callback));
};

class MockIpfsBasePinService : public IpfsBasePinService {
 public:
  MockIpfsBasePinService() = default;
  void AddJob(std::unique_ptr<IpfsBaseJob> job) override {
    std::move(job)->Start();
  }
};

}  // namespace

class IpfsLocalPinServiceTest : public testing::Test {
 public:
  IpfsLocalPinServiceTest() = default;

  IpfsLocalPinService* service() { return ipfs_local_pin_service_.get(); }

 protected:
  void SetUp() override {
    auto* registry = pref_service_.registry();
    IpfsService::RegisterProfilePrefs(registry);
    ipfs_local_pin_service_ =
        std::make_unique<IpfsLocalPinService>(GetPrefs(), GetIpfsService());
    ipfs_local_pin_service_->SetIpfsBasePinServiceForTesting(
        std::make_unique<MockIpfsBasePinService>());
  }

  PrefService* GetPrefs() { return &pref_service_; }

  testing::NiceMock<MockIpfsService>* GetIpfsService() {
    return &ipfs_service_;
  }

  std::unique_ptr<IpfsLocalPinService> ipfs_local_pin_service_;
  testing::NiceMock<MockIpfsService> ipfs_service_;
  TestingPrefServiceSimple pref_service_;
  content::BrowserTaskEnvironment task_environment_;
};

TEST_F(IpfsLocalPinServiceTest, AddLocalPinJobTest) {
  {
    ON_CALL(*GetIpfsService(), AddPin(_, _, _))
        .WillByDefault(::testing::Invoke(
            [](const std::vector<std::string>& cids, bool recursive,
               IpfsService::AddPinCallback callback) {
              AddPinResult result;
              if (recursive) {
                result.pins = {"Qjson", "Qimage"};
              } else {
                result.pins = {"Qma", "Qmb", "QmaD1", "QmbD1"};
              }
              result.recursive = recursive;
              std::move(callback).Run(result);
            }));

    std::optional<bool> success;
    service()->AddPins(
        "a", {"ipfs://Qma/metadata/1.json", "ipfs://Qmb/images/1.jpg"},
        base::BindLambdaForTesting(
            [&success](bool result) { success = result; }));

    std::string expected = R"({"recursive": {
                                "Qjson" : ["a"],
                                "Qimage" : ["a"]
                             }, "direct": {
                                "Qma" : ["a"],
                                "Qmb" : ["a"],
                                "QmaD1" : ["a"],
                                "QmbD1" : ["a"]
                             }})";
    std::optional<base::Value> expected_value = base::JSONReader::Read(
        expected, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                      base::JSONParserOptions::JSON_PARSE_RFC);
    EXPECT_EQ(expected_value.value(), GetPrefs()->GetDict(kIPFSPinnedCids));
    EXPECT_TRUE(success.value());
  }

  {
    ON_CALL(*GetIpfsService(), AddPin(_, _, _))
        .WillByDefault(::testing::Invoke(
            [](const std::vector<std::string>& cids, bool recursive,
               IpfsService::AddPinCallback callback) {
              AddPinResult result;
              if (recursive) {
                result.pins = {"Qma", "Qmb", "Qmc", "Qmd", "Qimage"};
              }
              result.recursive = recursive;
              std::move(callback).Run(result);
            }));

    std::optional<bool> success;
    service()->AddPins("b",
                       {"ipfs://Qma", "ipfs://Qmb", "ipfs://Qmc", "ipfs://Qmd",
                        "ipfs://Qimage"},
                       base::BindLambdaForTesting(
                           [&success](bool result) { success = result; }));

    std::string expected = R"({"recursive": {
                                "Qjson" : ["a"],
                                "Qimage" : ["a", "b"],
                                "Qma": ["b"],
                                "Qmb": ["b"],
                                "Qmc": ["b"],
                                "Qmd": ["b"]
                             }, "direct": {
                                "Qma" : ["a"],
                                "Qmb" : ["a"],
                                "QmaD1" : ["a"],
                                "QmbD1" : ["a"]
                             }})";
    std::optional<base::Value> expected_value = base::JSONReader::Read(
        expected, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                      base::JSONParserOptions::JSON_PARSE_RFC);
    EXPECT_EQ(expected_value.value(), GetPrefs()->GetDict(kIPFSPinnedCids));
    EXPECT_TRUE(success.value());
  }

  // Not all CIDS are pinned - pinning for "c" fails
  {
    ON_CALL(*GetIpfsService(), AddPin(_, _, _))
        .WillByDefault(::testing::Invoke(
            [](const std::vector<std::string>& cids, bool recursive,
               IpfsService::AddPinCallback callback) {
              AddPinResult result;
              if (recursive) {
                std::move(callback).Run(std::nullopt);
                return;
              }
              result.recursive = recursive;
              std::move(callback).Run(result);
            }));

    std::optional<bool> success;
    service()->AddPins(
        "c",
        {"ipfs://Qma", "ipfs://Qmb", "ipfs://Qmc", "ipfs://Qmd", "ipfs://Qme"},
        base::BindLambdaForTesting(
            [&success](bool result) { success = result; }));

    std::string expected = R"({"recursive": {
                                "Qjson" : ["a"],
                                "Qimage" : ["a", "b"],
                                "Qma": ["b"],
                                "Qmb": ["b"],
                                "Qmc": ["b"],
                                "Qmd": ["b"]
                             }, "direct": {
                                "Qma" : ["a"],
                                "Qmb" : ["a"],
                                "QmaD1" : ["a"],
                                "QmbD1" : ["a"]
                             }})";
    std::optional<base::Value> expected_value = base::JSONReader::Read(
        expected, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                      base::JSONParserOptions::JSON_PARSE_RFC);
    EXPECT_EQ(expected_value.value(), GetPrefs()->GetDict(kIPFSPinnedCids));
    EXPECT_FALSE(success.value());
  }

  // Nothing is pinned for "c"
  {
    ON_CALL(*GetIpfsService(), AddPin(_, _, _))
        .WillByDefault(::testing::Invoke(
            [](const std::vector<std::string>& cids, bool recursive,
               IpfsService::AddPinCallback callback) {
              std::move(callback).Run(std::nullopt);
            }));

    std::optional<bool> success;
    service()->AddPins(
        "c",
        {"ipfs://Qma", "ipfs://Qmb", "ipfs://Qmc", "ipfs://Qmd", "ipfs://Qme"},
        base::BindLambdaForTesting(
            [&success](bool result) { success = result; }));

    std::string expected = R"({"recursive": {
                                "Qjson" : ["a"],
                                "Qimage" : ["a", "b"],
                                "Qma": ["b"],
                                "Qmb": ["b"],
                                "Qmc": ["b"],
                                "Qmd": ["b"]
                             }, "direct": {
                                "Qma" : ["a"],
                                "Qmb" : ["a"],
                                "QmaD1" : ["a"],
                                "QmbD1" : ["a"]
                             }})";
    std::optional<base::Value> expected_value = base::JSONReader::Read(
        expected, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                      base::JSONParserOptions::JSON_PARSE_RFC);
    EXPECT_EQ(expected_value.value(), GetPrefs()->GetDict(kIPFSPinnedCids));
    EXPECT_FALSE(success.value());
  }
}

TEST_F(IpfsLocalPinServiceTest, RemoveLocalPinJobTest) {
  {
    std::string base = R"({"recursive": {
                                  "Qma" : ["a", "b"],
                                  "Qmb" : ["a", "b"],
                                  "Qmc" : ["a", "b"],
                                  "Qmd" : ["b"]
                               }, "direct": {}})";
    std::optional<base::Value> base_value = base::JSONReader::Read(
        base, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                  base::JSONParserOptions::JSON_PARSE_RFC);
    GetPrefs()->SetDict(kIPFSPinnedCids, base_value.value().GetDict().Clone());
  }

  {
    std::optional<bool> success;
    service()->RemovePins("a",
                          base::BindLambdaForTesting(
                              [&success](bool result) { success = result; }));

    std::string expected = R"({
                             "Qma" : ["b"],
                             "Qmb" : ["b"],
                             "Qmc" : ["b"],
                             "Qmd" : ["b"]
                             })";
    std::optional<base::Value> expected_value = base::JSONReader::Read(
        expected, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                      base::JSONParserOptions::JSON_PARSE_RFC);
    EXPECT_EQ(expected_value.value(),
              *(GetPrefs()->GetDict(kIPFSPinnedCids).FindDict("recursive")));
    EXPECT_TRUE(success.value());
  }

  {
    std::optional<bool> success;
    service()->RemovePins("c",
                          base::BindLambdaForTesting(
                              [&success](bool result) { success = result; }));

    std::string expected = R"({
                             "Qma" : ["b"],
                             "Qmb" : ["b"],
                             "Qmc" : ["b"],
                             "Qmd" : ["b"]
                             })";
    std::optional<base::Value> expected_value = base::JSONReader::Read(
        expected, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                      base::JSONParserOptions::JSON_PARSE_RFC);
    EXPECT_EQ(expected_value.value(),
              *(GetPrefs()->GetDict(kIPFSPinnedCids).FindDict("recursive")));
    EXPECT_TRUE(success.value());
  }

  {
    std::optional<bool> success;
    service()->RemovePins("b",
                          base::BindLambdaForTesting(
                              [&success](bool result) { success = result; }));

    std::string expected = R"({
                             })";
    std::optional<base::Value> expected_value = base::JSONReader::Read(
        expected, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                      base::JSONParserOptions::JSON_PARSE_RFC);
    EXPECT_EQ(expected_value.value(),
              *(GetPrefs()->GetDict(kIPFSPinnedCids).FindDict("recursive")));
    EXPECT_TRUE(success.value());
  }
}

TEST_F(IpfsLocalPinServiceTest, VerifyLocalPinJobTest) {
  {
    std::string base = R"({"recursive": {
                                  "Qma" : ["a", "b"],
                                  "Qmb" : ["a", "b"],
                                  "Qmc" : ["a", "b"],
                                  "Qmd" : ["b"]
                               }, "direct": {}})";
    std::optional<base::Value> base_value = base::JSONReader::Read(
        base, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                  base::JSONParserOptions::JSON_PARSE_RFC);
    GetPrefs()->SetDict(kIPFSPinnedCids, base_value.value().GetDict().Clone());
  }

  {
    ON_CALL(*GetIpfsService(), GetPins(_, _, _, _))
        .WillByDefault(::testing::Invoke([](const std::optional<
                                                std::vector<std::string>>& cid,
                                            const std::string& type, bool quiet,
                                            IpfsService::GetPinsCallback
                                                callback) {
          GetPinsResult result = {{"Qma", "Recursive"}, {"Qmb", "Recursive"}};
          std::move(callback).Run(result);
        }));

    std::optional<bool> success;
    service()->ValidatePins(
        "a", {"ipfs://Qma", "ipfs://Qmb", "ipfs://Qmc"},
        base::BindLambdaForTesting([&success](std::optional<bool> result) {
          success = result.value();
        }));

    EXPECT_TRUE(success.has_value());
    EXPECT_FALSE(success.value());
  }

  {
    ON_CALL(*GetIpfsService(), GetPins(_, _, _, _))
        .WillByDefault(::testing::Invoke(
            [](const std::optional<std::vector<std::string>>& cid,
               const std::string& type, bool quiet,
               IpfsService::GetPinsCallback callback) {
              GetPinsResult result = {{"Qma", "Recursive"},
                                      {"Qmb", "Recursive"},
                                      {"Qmc", "Recursive"}};
              std::move(callback).Run(result);
            }));

    std::optional<bool> success;
    service()->ValidatePins(
        "a", {"ipfs://Qma", "ipfs://Qmb", "ipfs://Qmc"},
        base::BindLambdaForTesting([&success](std::optional<bool> result) {
          success = result.value();
        }));
    EXPECT_TRUE(success.has_value());
    EXPECT_TRUE(success.value());
  }

  {
    ON_CALL(*GetIpfsService(), GetPins(_, _, _, _))
        .WillByDefault(::testing::Invoke(
            [](const std::optional<std::vector<std::string>>& cid,
               const std::string& type, bool quiet,
               IpfsService::GetPinsCallback callback) {
              GetPinsResult result = {};
              std::move(callback).Run(result);
            }));

    std::optional<bool> success = false;
    service()->ValidatePins(
        "b", {"ipfs://Qma", "ipfs://Qmb", "ipfs://Qmc", "ipfs://Qmd"},
        base::BindLambdaForTesting([&success](std::optional<bool> result) {
          success = result.value();
        }));

    EXPECT_TRUE(success.has_value());

    EXPECT_FALSE(success.value());
  }

  {
    std::optional<bool> success;
    VerifyLocalPinJob job(
        GetPrefs(), GetIpfsService(), "b",
        IpfsLocalPinService::ExtractMergedPinData(
            {"ipfs://Qma", "ipfs://Qmb", "ipfs://Qmc", "ipfs://Qmd"})
            .value(),
        base::BindLambdaForTesting(
            [&success](std::optional<bool> result) { success = result; }));

    ON_CALL(*GetIpfsService(), GetPins(_, _, _, _))
        .WillByDefault(::testing::Invoke(
            [](const std::optional<std::vector<std::string>>& cid,
               const std::string& type, bool quiet,
               IpfsService::GetPinsCallback callback) {
              std::move(callback).Run(std::nullopt);
            }));

    job.Start();

    EXPECT_FALSE(success.value());
  }
}

TEST_F(IpfsLocalPinServiceTest, GcJobTest) {
  {
    std::string base = R"({"recursive":{
                                  "Qma" : ["a", "b"],

                                  "Qmc" : ["a", "b"],
                                  "Qmd" : ["b"]
                               }, "direct":{
                                "Qmb" : ["a", "b"]
                                }})";
    std::optional<base::Value> base_value = base::JSONReader::Read(
        base, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                  base::JSONParserOptions::JSON_PARSE_RFC);
    GetPrefs()->SetDict(kIPFSPinnedCids, base_value.value().GetDict().Clone());
  }

  {
    std::optional<bool> success;
    GcJob job(GetPrefs(), GetIpfsService(),
              base::BindLambdaForTesting(
                  [&success](bool result) { success = result; }));

    ON_CALL(*GetIpfsService(), GetPins(_, _, _, _))
        .WillByDefault(::testing::Invoke(
            [](const std::optional<std::vector<std::string>>& cid,
               const std::string& type, bool quiet,
               IpfsService::GetPinsCallback callback) {
              EXPECT_FALSE(cid.has_value());
              EXPECT_TRUE(quiet);
              GetPinsResult result = {{"Qma", "Recursive"},
                                      {"Qmb", "Recursive"},
                                      {"Qmc", "Recursive"}};
              std::move(callback).Run(result);
            }));

    EXPECT_CALL(*GetIpfsService(), RemovePin(_, _)).Times(0);

    job.Start();

    EXPECT_TRUE(success.value());
  }

  {
    std::optional<bool> success;
    GcJob job(GetPrefs(), GetIpfsService(),
              base::BindLambdaForTesting(
                  [&success](bool result) { success = result; }));

    ON_CALL(*GetIpfsService(), GetPins(_, _, _, _))
        .WillByDefault(::testing::Invoke(
            [](const std::optional<std::vector<std::string>>& cid,
               const std::string& type, bool quiet,
               IpfsService::GetPinsCallback callback) {
              EXPECT_FALSE(cid.has_value());
              EXPECT_TRUE(quiet);
              GetPinsResult result;
              if (type == "recursive") {
                result = {{"Qm1", "Recursive"}, {"Qm2", "Recursive"}};
              } else {
                result = {};
              }
              std::move(callback).Run(result);
            }));
    EXPECT_CALL(*GetIpfsService(), RemovePin(_, _)).Times(1);

    ON_CALL(*GetIpfsService(), RemovePin(_, _))
        .WillByDefault(
            ::testing::Invoke([](const std::vector<std::string>& cid,
                                 IpfsService::RemovePinCallback callback) {
              EXPECT_EQ(cid.size(), 2u);
              EXPECT_EQ(cid[0], "Qm1");
              EXPECT_EQ(cid[1], "Qm2");
              RemovePinResult result = cid;
              std::move(callback).Run(result);
            }));

    job.Start();

    EXPECT_TRUE(success.value());
  }

  {
    std::optional<bool> success;
    GcJob job(GetPrefs(), GetIpfsService(),
              base::BindLambdaForTesting(
                  [&success](bool result) { success = result; }));

    ON_CALL(*GetIpfsService(), GetPins(_, _, _, _))
        .WillByDefault(::testing::Invoke(
            [](const std::optional<std::vector<std::string>>& cid,
               const std::string& type, bool quiet,
               IpfsService::GetPinsCallback callback) {
              std::move(callback).Run(std::nullopt);
            }));

    EXPECT_CALL(*GetIpfsService(), RemovePin(_, _)).Times(0);

    job.Start();

    EXPECT_FALSE(success.value());
  }
}

TEST_F(IpfsLocalPinServiceTest, ResetTest) {
  {
    std::string base = R"({"recursive":{
                                    "Qma" : ["a", "b"],
                                    "Qmb" : ["a", "b"],
                                    "Qmc" : ["a", "b"],
                                    "Qmd" : ["b"]
                                 }, "direct":{}})";
    std::optional<base::Value> base_value = base::JSONReader::Read(
        base, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                  base::JSONParserOptions::JSON_PARSE_RFC);
    GetPrefs()->SetDict(kIPFSPinnedCids, base_value.value().GetDict().Clone());
  }

  ON_CALL(*GetIpfsService(), LsPinCli(_))
      .WillByDefault(::testing::Invoke([](IpfsService::NodeCallback callback) {
        std::move(callback).Run("bafy1\nbafy2\nbafy3");
      }));
  ON_CALL(*GetIpfsService(), RemovePinCli(_, _))
      .WillByDefault(::testing::Invoke(
          [](std::set<std::string> cid, IpfsService::BoolCallback callback) {
            EXPECT_EQ(3u, cid.size());
            EXPECT_NE(cid.end(), cid.find("bafy1"));
            EXPECT_NE(cid.end(), cid.find("bafy2"));
            EXPECT_NE(cid.end(), cid.find("bafy3"));
            std::move(callback).Run(true);
          }));
  std::optional<bool> reset_result;
  service()->AddPins("123", {"ipfs://bafy1", "ipfs://bafy2"},
                     base::DoNothing());
  service()->AddPins("345", {"ipfs://bafy1", "ipfs://bafy2"},
                     base::DoNothing());
  service()->AddPins("567", {"ipfs://bafy1", "ipfs://bafy2"},
                     base::DoNothing());

  service()->Reset(base::BindLambdaForTesting(
      [&reset_result](bool result) { reset_result = result; }));
  EXPECT_EQ(0u, GetPrefs()->GetDict(kIPFSPinnedCids).size());
  EXPECT_FALSE(service()->HasJobs());
  ASSERT_TRUE(reset_result.value());
}

TEST_F(IpfsLocalPinServiceTest, ResetTest_UnpinFailed) {
  {
    std::string base = R"({"recursive": {
                                    "Qma" : ["a", "b"],
                                    "Qmb" : ["a", "b"],
                                    "Qmc" : ["a", "b"],
                                    "Qmd" : ["b"]
                                 }, "direct":{}})";
    std::optional<base::Value> base_value = base::JSONReader::Read(
        base, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                  base::JSONParserOptions::JSON_PARSE_RFC);
    GetPrefs()->SetDict(kIPFSPinnedCids, base_value.value().GetDict().Clone());
  }

  ON_CALL(*GetIpfsService(), LsPinCli(_))
      .WillByDefault(::testing::Invoke([](IpfsService::NodeCallback callback) {
        std::move(callback).Run("bafy1\nbafy2\nbafy3");
      }));
  ON_CALL(*GetIpfsService(), RemovePinCli(_, _))
      .WillByDefault(::testing::Invoke(
          [](std::set<std::string> cid, IpfsService::BoolCallback callback) {
            EXPECT_EQ(3u, cid.size());
            EXPECT_NE(cid.end(), cid.find("bafy1"));
            EXPECT_NE(cid.end(), cid.find("bafy2"));
            EXPECT_NE(cid.end(), cid.find("bafy3"));
            std::move(callback).Run(false);
          }));
  std::optional<bool> reset_result;
  service()->Reset(base::BindLambdaForTesting(
      [&reset_result](bool result) { reset_result = result; }));
  EXPECT_EQ(4u,
            GetPrefs()->GetDict(kIPFSPinnedCids).FindDict("recursive")->size());
  ASSERT_FALSE(reset_result.value());
}

TEST_F(IpfsLocalPinServiceTest, ResetTest_NoPinnedItems) {
  {
    std::string base = R"({"recursive":{
                                    "Qma" : ["a", "b"],
                                    "Qmb" : ["a", "b"],
                                    "Qmc" : ["a", "b"],
                                    "Qmd" : ["b"]
                                 }, "direct":{}})";
    std::optional<base::Value> base_value = base::JSONReader::Read(
        base, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                  base::JSONParserOptions::JSON_PARSE_RFC);
    GetPrefs()->SetDict(kIPFSPinnedCids, base_value.value().GetDict().Clone());
  }

  ON_CALL(*GetIpfsService(), LsPinCli(_))
      .WillByDefault(::testing::Invoke([](IpfsService::NodeCallback callback) {
        std::move(callback).Run("");
      }));
  std::optional<bool> reset_result;
  service()->Reset(base::BindLambdaForTesting(
      [&reset_result](bool result) { reset_result = result; }));
  EXPECT_EQ(0u, GetPrefs()->GetDict(kIPFSPinnedCids).size());
  ASSERT_TRUE(reset_result.value());
}

}  // namespace ipfs
