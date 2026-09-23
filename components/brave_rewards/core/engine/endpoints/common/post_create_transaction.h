/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENGINE_ENDPOINTS_COMMON_POST_CREATE_TRANSACTION_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENGINE_ENDPOINTS_COMMON_POST_CREATE_TRANSACTION_H_

#include <string>

#include "brave/components/brave_rewards/core/engine/endpoints/request_builder.h"
#include "brave/components/brave_rewards/core/mojom/rewards_engine_internal.mojom.h"

namespace brave_rewards::internal {
class RewardsEngine;

namespace endpoints {

class PostCreateTransaction : public RequestBuilder {
 public:
  PostCreateTransaction(RewardsEngine& engine,
                        std::string&& token,
                        std::string&& address,
                        mojom::ExternalTransactionPtr);
  ~PostCreateTransaction() override;

 private:
  std::string ContentType() const override;

 protected:
  // TODO(https://github.com/brave/brave-browser/issues/48713): This is a case
  // of `-Wexit-time-destructors` violation and `[[clang::no_destroy]]` has been
  // added in the meantime to fix the build error. Remove this attribute and
  // provide a proper fix.
  [[clang::no_destroy]] inline static const std::string kFeeMessage =
      "5% transaction fee collected by Brave Software International";

  std::string token_;
  std::string address_;
  mojom::ExternalTransactionPtr transaction_;
};

}  // namespace endpoints
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENGINE_ENDPOINTS_COMMON_POST_CREATE_TRANSACTION_H_
