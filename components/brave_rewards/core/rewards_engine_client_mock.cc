/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"

#include <optional>
#include <utility>

#include "brave/components/brave_rewards/core/test/test_rewards_engine_client.h"

using ::testing::_;

namespace brave_rewards::internal {

MockRewardsEngineClient::MockRewardsEngineClient() {
  // If a mock function takes a response callback as its parameter,
  // its default action won't run the callback passed to it, which is an error
  // if the callback's corresponding interface pipe is not closed. Make sure we
  // don't drop response callbacks by explicitly specifying default actions for
  // such functions.
  ON_CALL(*this, FetchFavIcon(_, _, _))
      .WillByDefault([](const std::string&, const std::string&, auto callback) {
        std::move(callback).Run(false, "");
      });

  ON_CALL(*this, LoadURL(_, _))
      .WillByDefault([](mojom::UrlRequestPtr, auto callback) {
        std::move(callback).Run(nullptr);
      });

  ON_CALL(*this, GetSPLTokenAccountBalance(_, _, _))
      .WillByDefault([](const std::string&, const std::string&, auto callback) {
        std::move(callback).Run(nullptr);
      });

  ON_CALL(*this, GetUserPreferenceValue(_, _))
      .WillByDefault([](const std::string&, auto callback) {
        std::move(callback).Run(base::Value());
      });

  ON_CALL(*this, SetUserPreferenceValue(_, _, _))
      .WillByDefault([](const std::string&, base::Value, auto callback) {
        std::move(callback).Run();
      });

  ON_CALL(*this, ClearUserPreferenceValue(_, _))
      .WillByDefault(
          [](const std::string&, auto callback) { std::move(callback).Run(); });

  ON_CALL(*this, ShowNotification(_, _, _))
      .WillByDefault([](const std::string&, const std::vector<std::string>&,
                        auto callback) {
        std::move(callback).Run(mojom::Result::FAILED);
      });

  ON_CALL(*this, RunDBTransaction(_, _))
      .WillByDefault([](mojom::DBTransactionPtr, auto callback) {
        std::move(callback).Run(nullptr);
      });

  ON_CALL(*this, DeleteLog(_)).WillByDefault([](auto callback) {
    std::move(callback).Run(mojom::Result::FAILED);
  });
}

MockRewardsEngineClient::~MockRewardsEngineClient() = default;

void MockRewardsEngineClient::EncryptString(const std::string& value,
                                            EncryptStringCallback callback) {
  std::move(callback).Run(FakeEncryption::EncryptString(value));
}

void MockRewardsEngineClient::DecryptString(const std::string& value,
                                            DecryptStringCallback callback) {
  std::move(callback).Run(FakeEncryption::DecryptString(value));
}

}  // namespace brave_rewards::internal
