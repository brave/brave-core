/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_HANDLER_H_

#include "brave/components/brave_origin/common/mojom/brave_origin_settings.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace brave_origin {
class BraveOriginService;

class BraveOriginSettingsHandlerImpl
    : public mojom::BraveOriginSettingsHandler {
 public:
  explicit BraveOriginSettingsHandlerImpl(
      BraveOriginService* brave_origin_service);
  ~BraveOriginSettingsHandlerImpl() override;

  BraveOriginSettingsHandlerImpl(const BraveOriginSettingsHandlerImpl&) =
      delete;
  BraveOriginSettingsHandlerImpl& operator=(
      const BraveOriginSettingsHandlerImpl&) = delete;

  void BindInterface(
      mojo::PendingReceiver<mojom::BraveOriginSettingsHandler> receiver);

  // mojom::BraveOriginSettingsHandler:
  void IsBraveOriginUser(IsBraveOriginUserCallback callback) override;
  void IsPolicyControlledByBraveOrigin(
      const std::string& policy_key,
      IsPolicyControlledByBraveOriginCallback callback) override;
  void GetPolicyValue(const std::string& policy_key,
                      GetPolicyValueCallback callback) override;
  void SetPolicyValue(const std::string& policy_key,
                      bool value,
                      SetPolicyValueCallback callback) override;

 private:
  raw_ptr<BraveOriginService> brave_origin_service_;
  mojo::Receiver<mojom::BraveOriginSettingsHandler> receiver_{this};
};

}  // namespace brave_origin

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_HANDLER_H_
