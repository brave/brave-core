/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_REQUEST_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_REQUEST_BUILDER_H_

#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/core/mojom_structs.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace endpoints {

class RequestBuilder {
 public:
  static constexpr char kApplicationJson[] = "application/json; charset=utf-8";

  virtual ~RequestBuilder();

  absl::optional<mojom::UrlRequestPtr> Request() const;

 protected:
  explicit RequestBuilder(RewardsEngineImpl& engine);

  virtual absl::optional<std::string> Url() const = 0;

  virtual mojom::UrlMethod Method() const;

  virtual absl::optional<std::vector<std::string>> Headers(
      const std::string& content) const;

  virtual absl::optional<std::string> Content() const;

  virtual std::string ContentType() const;

  virtual bool SkipLog() const;

  virtual uint32_t LoadFlags() const;

  const raw_ref<RewardsEngineImpl> engine_;
};

}  // namespace endpoints
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_REQUEST_BUILDER_H_
