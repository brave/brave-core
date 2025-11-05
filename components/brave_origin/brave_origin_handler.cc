/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_handler.h"

#include "brave/components/brave_origin/brave_origin_service.h"
#include "brave/components/brave_origin/brave_origin_utils.h"

namespace brave_origin {

BraveOriginSettingsHandlerImpl::BraveOriginSettingsHandlerImpl(
    BraveOriginService* brave_origin_service)
    : brave_origin_service_(brave_origin_service) {
  CHECK(brave_origin_service_);
}

BraveOriginSettingsHandlerImpl::~BraveOriginSettingsHandlerImpl() = default;

void BraveOriginSettingsHandlerImpl::BindInterface(
    mojo::PendingReceiver<mojom::BraveOriginSettingsHandler> receiver) {
  receiver_.reset();
  receiver_.Bind(std::move(receiver));
}

void BraveOriginSettingsHandlerImpl::IsBraveOriginUser(
    IsBraveOriginUserCallback callback) {
  bool is_brave_origin_user = brave_origin::IsBraveOriginEnabled();
  std::move(callback).Run(is_brave_origin_user);
}

void BraveOriginSettingsHandlerImpl::IsPolicyControlledByBraveOrigin(
    const std::string& policy_key,
    IsPolicyControlledByBraveOriginCallback callback) {
  bool is_controlled =
      brave_origin_service_->IsPolicyControlledByBraveOrigin(policy_key);
  std::move(callback).Run(is_controlled);
}

void BraveOriginSettingsHandlerImpl::GetPolicyValue(
    const std::string& policy_key,
    GetPolicyValueCallback callback) {
  if (!brave_origin::IsBraveOriginEnabled()) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  std::optional<bool> value = brave_origin_service_->GetPolicyValue(policy_key);
  std::move(callback).Run(value);
}

void BraveOriginSettingsHandlerImpl::SetPolicyValue(
    const std::string& policy_key,
    bool value,
    SetPolicyValueCallback callback) {
  if (!brave_origin::IsBraveOriginEnabled()) {
    std::move(callback).Run(false);
    return;
  }

  bool success = brave_origin_service_->SetPolicyValue(policy_key, value);
  std::move(callback).Run(success);
}

}  // namespace brave_origin
