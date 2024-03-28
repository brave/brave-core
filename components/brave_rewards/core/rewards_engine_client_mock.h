/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_CLIENT_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_CLIENT_MOCK_H_

#include <string>
#include <vector>

#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/common/mojom/rewards_database.mojom.h"
#include "brave/components/brave_rewards/common/mojom/rewards_engine.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_rewards::internal {

inline const auto db_error_response = [] {
  auto response = mojom::DBCommandResponse::New();
  response->status = mojom::DBCommandResponse::Status::RESPONSE_ERROR;
  return response;
}();

class MockRewardsEngineClient : public mojom::RewardsEngineClient {
 public:
  MockRewardsEngineClient();

  ~MockRewardsEngineClient() override;

  MOCK_METHOD2(OnReconcileComplete,
               void(mojom::Result, mojom::ContributionInfoPtr));

  MOCK_METHOD3(OnPanelPublisherInfo,
               void(mojom::Result, mojom::PublisherInfoPtr, uint64_t));

  MOCK_METHOD3(FetchFavIcon,
               void(const std::string&,
                    const std::string&,
                    FetchFavIconCallback));

  MOCK_METHOD2(LoadURL, void(mojom::UrlRequestPtr, LoadURLCallback));

  MOCK_METHOD3(GetSPLTokenAccountBalance,
               void(const std::string&,
                    const std::string&,
                    GetSPLTokenAccountBalanceCallback));

  MOCK_METHOD1(PublisherListNormalized,
               void(std::vector<mojom::PublisherInfoPtr>));

  MOCK_METHOD0(OnPublisherRegistryUpdated, void());

  MOCK_METHOD1(OnPublisherUpdated, void(const std::string&));

  MOCK_METHOD2(GetUserPreferenceValue,
               void(const std::string&, GetUserPreferenceValueCallback));

  MOCK_METHOD3(SetUserPreferenceValue,
               void(const std::string&,
                    base::Value,
                    SetUserPreferenceValueCallback));

  MOCK_METHOD2(ClearUserPreferenceValue,
               void(const std::string&, ClearUserPreferenceValueCallback));

  MOCK_METHOD3(ShowNotification,
               void(const std::string&,
                    const std::vector<std::string>&,
                    ShowNotificationCallback));

  MOCK_METHOD0(ReconcileStampReset, void());

  MOCK_METHOD2(RunDBTransaction,
               void(mojom::DBTransactionPtr, RunDBTransactionCallback));

  MOCK_METHOD1(PendingContributionSaved, void(mojom::Result));

  MOCK_METHOD4(Log,
               void(const std::string&, int32_t, int32_t, const std::string&));

  MOCK_METHOD0(ClearAllNotifications, void());

  MOCK_METHOD0(ExternalWalletConnected, void());

  MOCK_METHOD0(ExternalWalletLoggedOut, void());

  MOCK_METHOD0(ExternalWalletReconnected, void());

  MOCK_METHOD0(ExternalWalletDisconnected, void());

  MOCK_METHOD1(DeleteLog, void(DeleteLogCallback));

  void EncryptString(const std::string& value,
                     EncryptStringCallback callback) override;

  void DecryptString(const std::string& value,
                     DecryptStringCallback callback) override;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_CLIENT_MOCK_H_
