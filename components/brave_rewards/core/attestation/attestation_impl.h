/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ATTESTATION_ATTESTATION_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ATTESTATION_ATTESTATION_IMPL_H_

#include <memory>
#include <string>

#include "brave/components/brave_rewards/core/attestation/attestation.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace attestation {

class AttestationImpl : public Attestation {
 public:
  explicit AttestationImpl(RewardsEngineImpl& engine);
  ~AttestationImpl() override;

  void Start(const std::string& payload, StartCallback callback) override;

  void Confirm(const std::string& solution, ConfirmCallback callback) override;

 private:
  std::unique_ptr<Attestation> platform_instance_;
};

}  // namespace attestation
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ATTESTATION_ATTESTATION_IMPL_H_
