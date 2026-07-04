/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_PURCHASED_STATE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_PURCHASED_STATE_MANAGER_H_

#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"

class PrefService;

namespace brave_vpn::v2 {

// PurchasedStateManager owns the purchased-state value and the current
// environment, and runs the credential chain that produces that state.
// For now, it is a stub.
class PurchasedStateManager final {
 public:
  using PurchasedStateChangedCallback =
      base::RepeatingCallback<void(mojom::PurchasedState,
                                   std::optional<std::string>)>;

  PurchasedStateManager(PrefService* local_prefs,
                        PurchasedStateChangedCallback callback);
  ~PurchasedStateManager();

  PurchasedStateManager(const PurchasedStateManager&) = delete;
  PurchasedStateManager& operator=(const PurchasedStateManager&) = delete;

  void Reload();
  void Load(const std::string& domain);
  mojom::PurchasedInfo GetInfo() const;
  bool IsPurchased() const;
  std::string GetCurrentEnvironment() const;

  void SetPurchasedState(const std::string& env,
                         mojom::PurchasedState state,
                         std::optional<std::string> description = std::nullopt);

 private:
  const raw_ref<PrefService> local_prefs_;
  PurchasedStateChangedCallback purchased_state_changed_callback_;
  std::optional<mojom::PurchasedInfo> purchased_state_;
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_PURCHASED_STATE_MANAGER_H_
