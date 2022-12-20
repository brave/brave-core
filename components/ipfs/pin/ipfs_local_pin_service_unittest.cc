// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ipfs/pin/ipfs_local_pin_service.h"

#include <memory>
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
               void(const absl::optional<std::vector<std::string>>& cid,
                    const std::string& type,
                    bool quiet,
                    IpfsService::GetPinsCallback callback));
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
              result.pins = cids;
              std::move(callback).Run(result);
            }));

    absl::optional<bool> success;
    service()->AddPins("a", {"Qma", "Qmb", "Qmc"},
                       base::BindLambdaForTesting(
                           [&success](bool result) { success = result; }));

    std::string expected = R"({
                                "Qma" : ["a"],
                                "Qmb" : ["a"],
                                "Qmc" : ["a"]
                             })";
    absl::optional<base::Value> expected_value = base::JSONReader::Read(
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
              result.pins = cids;
              std::move(callback).Run(result);
            }));

    absl::optional<bool> success;
    service()->AddPins("b", {"Qma", "Qmb", "Qmc", "Qmd"},
                       base::BindLambdaForTesting(
                           [&success](bool result) { success = result; }));

    std::string expected = R"({
                                "Qma" : ["a", "b"],
                                "Qmb" : ["a", "b"],
                                "Qmc" : ["a", "b"],
                                "Qmd" : ["b"]
                             })";
    absl::optional<base::Value> expected_value = base::JSONReader::Read(
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
              result.pins = {"Qma", "Qmb", "Qmc"};
              std::move(callback).Run(result);
            }));

    absl::optional<bool> success;
    service()->AddPins("c", {"Qma", "Qmb", "Qmc", "Qmd", "Qme"},
                       base::BindLambdaForTesting(
                           [&success](bool result) { success = result; }));

    std::string expected = R"({
                                "Qma" : ["a", "b"],
                                "Qmb" : ["a", "b"],
                                "Qmc" : ["a", "b"],
                                "Qmd" : ["b"]
                             })";
    absl::optional<base::Value> expected_value = base::JSONReader::Read(
        expected, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                      base::JSONParserOptions::JSON_PARSE_RFC);
    EXPECT_EQ(expected_value.value(), GetPrefs()->GetDict(kIPFSPinnedCids));
    EXPECT_FALSE(success.value());
  }

  {
    ON_CALL(*GetIpfsService(), AddPin(_, _, _))
        .WillByDefault(::testing::Invoke(
            [](const std::vector<std::string>& cids, bool recursive,
               IpfsService::AddPinCallback callback) {
              std::move(callback).Run(absl::nullopt);
            }));

    absl::optional<bool> success;
    service()->AddPins("c", {"Qma", "Qmb", "Qmc", "Qmd", "Qme"},
                       base::BindLambdaForTesting(
                           [&success](bool result) { success = result; }));

    std::string expected = R"({
                                "Qma" : ["a", "b"],
                                "Qmb" : ["a", "b"],
                                "Qmc" : ["a", "b"],
                                "Qmd" : ["b"]
                             })";
    absl::optional<base::Value> expected_value = base::JSONReader::Read(
        expected, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                      base::JSONParserOptions::JSON_PARSE_RFC);
    EXPECT_EQ(expected_value.value(), GetPrefs()->GetDict(kIPFSPinnedCids));
    EXPECT_FALSE(success.value());
  }
}

TEST_F(IpfsLocalPinServiceTest, RemoveLocalPinJobTest) {
  {
    std::string base = R"({
                                  "Qma" : ["a", "b"],
                                  "Qmb" : ["a", "b"],
                                  "Qmc" : ["a", "b"],
                                  "Qmd" : ["b"]
                               })";
    absl::optional<base::Value> base_value = base::JSONReader::Read(
        base, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                  base::JSONParserOptions::JSON_PARSE_RFC);
    GetPrefs()->SetDict(kIPFSPinnedCids, base_value.value().GetDict().Clone());
  }

  {
    absl::optional<bool> success;
    service()->RemovePins("a",
                          base::BindLambdaForTesting(
                              [&success](bool result) { success = result; }));

    std::string expected = R"({
                             "Qma" : ["b"],
                             "Qmb" : ["b"],
                             "Qmc" : ["b"],
                             "Qmd" : ["b"]
                             })";
    absl::optional<base::Value> expected_value = base::JSONReader::Read(
        expected, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                      base::JSONParserOptions::JSON_PARSE_RFC);
    EXPECT_EQ(expected_value.value(), GetPrefs()->GetDict(kIPFSPinnedCids));
    EXPECT_TRUE(success.value());
  }

  {
    absl::optional<bool> success;
    service()->RemovePins("c",
                          base::BindLambdaForTesting(
                              [&success](bool result) { success = result; }));

    std::string expected = R"({
                             "Qma" : ["b"],
                             "Qmb" : ["b"],
                             "Qmc" : ["b"],
                             "Qmd" : ["b"]
                             })";
    absl::optional<base::Value> expected_value = base::JSONReader::Read(
        expected, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                      base::JSONParserOptions::JSON_PARSE_RFC);
    EXPECT_EQ(expected_value.value(), GetPrefs()->GetDict(kIPFSPinnedCids));
    EXPECT_TRUE(success.value());
  }

  {
    absl::optional<bool> success;
    service()->RemovePins("b",
                          base::BindLambdaForTesting(
                              [&success](bool result) { success = result; }));

    std::string expected = R"({
                             })";
    absl::optional<base::Value> expected_value = base::JSONReader::Read(
        expected, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                      base::JSONParserOptions::JSON_PARSE_RFC);
    EXPECT_EQ(expected_value.value(), GetPrefs()->GetDict(kIPFSPinnedCids));
    EXPECT_TRUE(success.value());
  }
}

TEST_F(IpfsLocalPinServiceTest, VerifyLocalPinJobTest) {
  {
    std::string base = R"({
                                  "Qma" : ["a", "b"],
                                  "Qmb" : ["a", "b"],
                                  "Qmc" : ["a", "b"],
                                  "Qmd" : ["b"]
                               })";
    absl::optional<base::Value> base_value = base::JSONReader::Read(
        base, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                  base::JSONParserOptions::JSON_PARSE_RFC);
    GetPrefs()->SetDict(kIPFSPinnedCids, base_value.value().GetDict().Clone());
  }

  {
    ON_CALL(*GetIpfsService(), GetPins(_, _, _, _))
        .WillByDefault(::testing::Invoke([](const absl::optional<
                                                std::vector<std::string>>& cid,
                                            const std::string& type, bool quiet,
                                            IpfsService::GetPinsCallback
                                                callback) {
          GetPinsResult result = {{"Qma", "Recursive"}, {"Qmb", "Recursive"}};
          std::move(callback).Run(result);
        }));

    absl::optional<bool> success;
    service()->ValidatePins(
        "a", {"Qma", "Qmb", "Qmc"},
        base::BindLambdaForTesting([&success](absl::optional<bool> result) {
          success = result.value();
        }));

    EXPECT_TRUE(success.has_value());
    EXPECT_FALSE(success.value());
  }

  {
    ON_CALL(*GetIpfsService(), GetPins(_, _, _, _))
        .WillByDefault(::testing::Invoke(
            [](const absl::optional<std::vector<std::string>>& cid,
               const std::string& type, bool quiet,
               IpfsService::GetPinsCallback callback) {
              GetPinsResult result = {{"Qma", "Recursive"},
                                      {"Qmb", "Recursive"},
                                      {"Qmc", "Recursive"}};
              std::move(callback).Run(result);
            }));

    absl::optional<bool> success;
    service()->ValidatePins(
        "a", {"Qma", "Qmb", "Qmc"},
        base::BindLambdaForTesting([&success](absl::optional<bool> result) {
          success = result.value();
        }));
    EXPECT_TRUE(success.has_value());
    EXPECT_TRUE(success.value());
  }

  {
    ON_CALL(*GetIpfsService(), GetPins(_, _, _, _))
        .WillByDefault(::testing::Invoke(
            [](const absl::optional<std::vector<std::string>>& cid,
               const std::string& type, bool quiet,
               IpfsService::GetPinsCallback callback) {
              GetPinsResult result = {};
              std::move(callback).Run(result);
            }));

    absl::optional<bool> success = false;
    service()->ValidatePins(
        "b", {"Qma", "Qmb", "Qmc", "Qmd"},
        base::BindLambdaForTesting([&success](absl::optional<bool> result) {
          success = result.value();
        }));

    EXPECT_TRUE(success.has_value());

    EXPECT_FALSE(success.value());
  }

  {
    absl::optional<bool> success;
    VerifyLocalPinJob job(
        GetPrefs(), GetIpfsService(), "b", {"Qma", "Qmb", "Qmc", "Qmd"},
        base::BindLambdaForTesting(
            [&success](absl::optional<bool> result) { success = result; }));

    ON_CALL(*GetIpfsService(), GetPins(_, _, _, _))
        .WillByDefault(::testing::Invoke(
            [](const absl::optional<std::vector<std::string>>& cid,
               const std::string& type, bool quiet,
               IpfsService::GetPinsCallback callback) {
              std::move(callback).Run(absl::nullopt);
            }));

    job.Start();

    EXPECT_FALSE(success.has_value());
  }
}

TEST_F(IpfsLocalPinServiceTest, GcJobTest) {
  {
    std::string base = R"({
                                  "Qma" : ["a", "b"],
                                  "Qmb" : ["a", "b"],
                                  "Qmc" : ["a", "b"],
                                  "Qmd" : ["b"]
                               })";
    absl::optional<base::Value> base_value = base::JSONReader::Read(
        base, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                  base::JSONParserOptions::JSON_PARSE_RFC);
    GetPrefs()->SetDict(kIPFSPinnedCids, base_value.value().GetDict().Clone());
  }

  {
    absl::optional<bool> success;
    GcJob job(GetPrefs(), GetIpfsService(),
              base::BindLambdaForTesting(
                  [&success](bool result) { success = result; }));

    ON_CALL(*GetIpfsService(), GetPins(_, _, _, _))
        .WillByDefault(::testing::Invoke(
            [](const absl::optional<std::vector<std::string>>& cid,
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
    absl::optional<bool> success;
    GcJob job(GetPrefs(), GetIpfsService(),
              base::BindLambdaForTesting(
                  [&success](bool result) { success = result; }));

    ON_CALL(*GetIpfsService(), GetPins(_, _, _, _))
        .WillByDefault(::testing::Invoke([](const absl::optional<
                                                std::vector<std::string>>& cid,
                                            const std::string& type, bool quiet,
                                            IpfsService::GetPinsCallback
                                                callback) {
          EXPECT_FALSE(cid.has_value());
          EXPECT_TRUE(quiet);
          GetPinsResult result = {{"Qm1", "Recursive"}, {"Qm2", "Recursive"}};
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
    absl::optional<bool> success;
    GcJob job(GetPrefs(), GetIpfsService(),
              base::BindLambdaForTesting(
                  [&success](bool result) { success = result; }));

    ON_CALL(*GetIpfsService(), GetPins(_, _, _, _))
        .WillByDefault(::testing::Invoke(
            [](const absl::optional<std::vector<std::string>>& cid,
               const std::string& type, bool quiet,
               IpfsService::GetPinsCallback callback) {
              std::move(callback).Run(absl::nullopt);
            }));

    EXPECT_CALL(*GetIpfsService(), RemovePin(_, _)).Times(0);

    job.Start();

    EXPECT_FALSE(success.value());
  }
}

}  // namespace ipfs
