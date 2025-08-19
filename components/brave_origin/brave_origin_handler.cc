/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_handler.h"

#include "brave/components/brave_origin/brave_origin_service.h"

namespace brave_origin {

BraveOriginSettingsHandlerImpl::BraveOriginSettingsHandlerImpl(
    BraveOriginService* brave_origin_service)
    : brave_origin_service_(brave_origin_service) {
  DCHECK(brave_origin_service_);
}

BraveOriginSettingsHandlerImpl::~BraveOriginSettingsHandlerImpl() = default;

void BraveOriginSettingsHandlerImpl::BindInterface(
    mojo::PendingReceiver<mojom::BraveOriginSettingsHandler> receiver) {
  receiver_.reset();
  receiver_.Bind(std::move(receiver));
}

void BraveOriginSettingsHandlerImpl::IsBraveOriginUser(
    IsBraveOriginUserCallback callback) {
  bool is_brave_origin_user = brave_origin_service_->IsBraveOriginUser();
  std::move(callback).Run(is_brave_origin_user);
}

void BraveOriginSettingsHandlerImpl::IsPrefControlledByBraveOrigin(
    const std::string& pref_name,
    IsPrefControlledByBraveOriginCallback callback) {
  bool is_controlled =
      brave_origin_service_->IsPrefControlledByBraveOrigin(pref_name);
  std::move(callback).Run(is_controlled);
}

void BraveOriginSettingsHandlerImpl::GetBraveOriginPref(
    const std::string& pref_name,
    GetBraveOriginPrefCallback callback) {
  if (!brave_origin_service_->IsBraveOriginUser()) {
    std::move(callback).Run(base::Value());
    return;
  }

  base::Value pref_value =
      brave_origin_service_->GetBraveOriginPrefValue(pref_name);
  std::move(callback).Run(std::move(pref_value));
}

void BraveOriginSettingsHandlerImpl::SetBraveOriginPref(
    const std::string& pref_name,
    base::Value value,
    SetBraveOriginPrefCallback callback) {
  if (!brave_origin_service_->IsBraveOriginUser() ||
      !brave_origin_service_->IsPrefControlledByBraveOrigin(pref_name)) {
    std::move(callback).Run(false);
    return;
  }

  bool success = brave_origin_service_->SetBraveOriginPolicyValue(
      pref_name, std::move(value));
  std::move(callback).Run(success);
}

}  // namespace brave_origin
