/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_COMMON_GET_TRANSACTION_STATUS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_COMMON_GET_TRANSACTION_STATUS_H_

#include <string>

#include "brave/components/brave_rewards/core/endpoints/request_builder.h"

namespace brave_rewards::internal {
class RewardsEngine;

namespace endpoints {

class GetTransactionStatus : public RequestBuilder {
 public:
  GetTransactionStatus(RewardsEngine& engine,
                       std::string&& token,
                       std::string&& transaction_id);
  ~GetTransactionStatus() override;

 private:
  mojom::UrlMethod Method() const override;

 protected:
  std::string token_;
  std::string transaction_id_;
};

}  // namespace endpoints
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_COMMON_GET_TRANSACTION_STATUS_H_
