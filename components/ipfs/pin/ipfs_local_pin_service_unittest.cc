/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/pin/ipfs_local_pin_service.h"

#include <memory>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
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

class X {
public:
    X() {}
    virtual void A() {
    }
    ~X() {}
};

class Y : public X {
public:
  MOCK_METHOD0(A, void());
};

class MockIpfsService : public IpfsService {
 public:
    MOCK_METHOD(void,
                RemovePin,
                (const std::vector<std::string>&, IpfsService::RemovePinCallback),
                (override));
    MOCK_METHOD(void, XXX, (), (override));
};

class MockIpfsService2 : public IpfsService {
 public:
    MockIpfsService2() :IpfsService() {}

    ~MockIpfsService2() override {}


  void RemovePin(const std::vector<std::string>& cid,
                 IpfsService::RemovePinCallback callback) override {
        ASSERT_TRUE(false);
     }


};

class MockIpfsBasePinService : public IpfsBasePinService {
 public:
  MockIpfsBasePinService() = default;
  void AddJob(std::unique_ptr<IpfsBaseJob> job) override {
    std::move(job)->Start();
  }
};

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

  MockIpfsService* GetIpfsService() {
    return &ipfs_service_;
  }

  std::unique_ptr<IpfsLocalPinService> ipfs_local_pin_service_;
  MockIpfsService ipfs_service_;
  TestingPrefServiceSimple pref_service_;
  content::BrowserTaskEnvironment task_environment_;
};

TEST_F(IpfsLocalPinServiceTest, test1) {
    testing::NiceMock<Y> y;
    ON_CALL(y, A())
        .WillByDefault(::testing::Invoke(
            []() {
              ASSERT_TRUE(false);
            }));
    static_cast<X*>(&y)->A();
}


TEST_F(IpfsLocalPinServiceTest, test2) {
    testing::NiceMock<MockIpfsService> y;
    ON_CALL(y, RemovePin(_, _))
        .WillByDefault(::testing::Invoke([](const std::vector<std::string>& cids,
               IpfsService::RemovePinCallback callback) {
                ASSERT_TRUE(false);
            }));
    static_cast<IpfsService*>(&y)->RemovePin({},
                                             base::BindLambdaForTesting([](bool, absl::optional<RemovePinResult>){}));
}

TEST_F(IpfsLocalPinServiceTest, test3) {
    testing::NiceMock<MockIpfsService> y;
    ON_CALL(y, XXX())
        .WillByDefault(::testing::Invoke([]() {
                ASSERT_TRUE(false);
            }));
    static_cast<IpfsService*>(&y)->XXX();
}

TEST_F(IpfsLocalPinServiceTest, AddLocalPinJobTest) {
  {

//    GetIpfsService()->RemovePin({},
//        base::BindLambdaForTesting([](bool, absl::optional<RemovePinResult>){}));

        ON_CALL(*GetIpfsService(), RemovePin(_, _))
            .WillByDefault([](const std::vector<std::string>& cids,
                   IpfsService::RemovePinCallback callback) {
                  LOG(ERROR) << "XXXZZZ service result";
                    ASSERT_TRUE(false);
                });
   EXPECT_CALL(*GetIpfsService(), RemovePin(_, _));
    std::vector<std::string> v;
   (*GetIpfsService()).RemovePin(v,
                    base::BindLambdaForTesting([](bool, absl::optional<RemovePinResult>){}));
   // EXPECT_CALL(*GetIpfsService(), RemovePin(_, _)).Times(1);

    LOG(ERROR) << "XXXZZZ service result " << GetIpfsService();

//    absl::optional<bool> success;
//    service()->AddPins("a", {"Qma", "Qmb", "Qmc"},
//                       base::BindLambdaForTesting(
//                           [&success](bool result) { success = result; }));

//    std::string expected = R"({
//                                "Qma" : ["a"],
//                                "Qmb" : ["a"],
//                                "Qmc" : ["a"]
//                             })";
//    absl::optional<base::Value> expected_value = base::JSONReader::Read(
//        expected, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
//                      base::JSONParserOptions::JSON_PARSE_RFC);
//    EXPECT_EQ(expected_value.value(), GetPrefs()->GetDict(kIPFSPinnedCids));
   // EXPECT_TRUE(success.value());
  }
}

TEST_F(IpfsLocalPinServiceTest, AddLocalPinJobTest_1) {
  {
    ON_CALL(*GetIpfsService(), XXX())
        .WillByDefault([]() {
              LOG(ERROR) << "XXXZZZ xxx1";
                ASSERT_TRUE(false);
            });

//    GetIpfsService()->RemovePin({},
//        base::BindLambdaForTesting([](bool, absl::optional<RemovePinResult>){}));

    static_cast<IpfsService*>(GetIpfsService())->XXX();
   // EXPECT_CALL(*GetIpfsService(), RemovePin(_, _)).Times(1);

    LOG(ERROR) << "XXXZZZ service result " << GetIpfsService();

//    absl::optional<bool> success;
//    service()->AddPins("a", {"Qma", "Qmb", "Qmc"},
//                       base::BindLambdaForTesting(
//                           [&success](bool result) { success = result; }));

//    std::string expected = R"({
//                                "Qma" : ["a"],
//                                "Qmb" : ["a"],
//                                "Qmc" : ["a"]
//                             })";
//    absl::optional<base::Value> expected_value = base::JSONReader::Read(
//        expected, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
//                      base::JSONParserOptions::JSON_PARSE_RFC);
//    EXPECT_EQ(expected_value.value(), GetPrefs()->GetDict(kIPFSPinnedCids));
   // EXPECT_TRUE(success.value());
  }
}


//TEST_F(IpfsLocalPinServiceTest, AddLocalPinJobTest) {
//  {
//    ON_CALL(*GetIpfsService(), AddPin(_, _, _))
//        .WillByDefault(::testing::Invoke(
//            [](const std::vector<std::string>& cids, bool recursive,
//               IpfsService::AddPinsCallback callback) {
//              LOG(ERROR) << "XXXZZZ service result";
//              AddPinResult result;
//              result.pins = cids;
//              std::move(callback).Run(true, result);
//            }));

//    GetIpfsService()->AddPin({}, true,
//        base::BindLambdaForTesting([](bool, absl::optional<AddPinResult>){}));

//    EXPECT_CALL(*GetIpfsService(), AddPin(_, _, _)).Times(1);

//    LOG(ERROR) << "XXXZZZ service result " << GetIpfsService();

////    absl::optional<bool> success;
////    service()->AddPins("a", {"Qma", "Qmb", "Qmc"},
////                       base::BindLambdaForTesting(
////                           [&success](bool result) { success = result; }));

//    std::string expected = R"({
//                                "Qma" : ["a"],
//                                "Qmb" : ["a"],
//                                "Qmc" : ["a"]
//                             })";
//    absl::optional<base::Value> expected_value = base::JSONReader::Read(
//        expected, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
//                      base::JSONParserOptions::JSON_PARSE_RFC);
//    EXPECT_EQ(expected_value.value(), GetPrefs()->GetDict(kIPFSPinnedCids));
//   // EXPECT_TRUE(success.value());
//  }
//}

}  // namespace ipfs
