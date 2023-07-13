/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "brave/components/brave_rewards/core/attestation/attestation_impl.h"

#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "build/build_config.h"
#include "net/http/http_status_code.h"

#if BUILDFLAG(IS_IOS)
#include "brave/components/brave_rewards/core/attestation/attestation_iosx.h"
#elif BUILDFLAG(IS_ANDROID)
#include "brave/components/brave_rewards/core/attestation/attestation_androidx.h"
#else
#include "brave/components/brave_rewards/core/attestation/attestation_desktop.h"
#endif

namespace brave_rewards::internal {
namespace attestation {

AttestationImpl::AttestationImpl(RewardsEngineImpl& engine)
    : Attestation(engine) {
#if BUILDFLAG(IS_IOS)
  platform_instance_ = std::make_unique<AttestationIOS>(engine);
#elif BUILDFLAG(IS_ANDROID)
  platform_instance_ = std::make_unique<AttestationAndroid>(engine);
#else
  platform_instance_ = std::make_unique<AttestationDesktop>(engine);
#endif
}

AttestationImpl::~AttestationImpl() = default;

void AttestationImpl::Start(const std::string& payload,
                            StartCallback callback) {
  platform_instance_->Start(payload, std::move(callback));
}

void AttestationImpl::Confirm(const std::string& solution,
                              ConfirmCallback callback) {
  platform_instance_->Confirm(solution, std::move(callback));
}

}  // namespace attestation
}  // namespace brave_rewards::internal
