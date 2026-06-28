/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_PURCHASED_STATE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_PURCHASED_STATE_MANAGER_H_

#include <memory>
#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "brave/components/skus/common/skus_sdk.mojom-forward.h"
#include "build/build_config.h"

class PrefService;

namespace brave_vpn::v2 {

class CredentialStore;
class SkusServiceClient;

// PurchasedStateManager owns the purchased-state value and the current
// environment, and runs the credential chain that produces that state. Also
// runs a timer that refreshes the subscriber credential before it expires.
class PurchasedStateManager final {
 public:
  using PurchasedStateChangedCallback =
      base::RepeatingCallback<void(mojom::PurchasedState,
                                   const std::optional<std::string>&)>;

  PurchasedStateManager(PrefService* local_prefs,
                        SkusServiceClient* skus_client,
                        PurchasedStateChangedCallback callback);
  ~PurchasedStateManager();

  PurchasedStateManager(const PurchasedStateManager&) = delete;
  PurchasedStateManager& operator=(const PurchasedStateManager&) = delete;

  void Reload();
  void Load(const std::string& domain);
  mojom::PurchasedInfo GetInfo() const;
  bool IsPurchased() const;
  std::string GetCurrentEnvironment() const;

  void SetPurchasedState(
      const std::string& env,
      mojom::PurchasedState state,
      const std::optional<std::string>& description = std::nullopt);

 private:
  friend class PurchasedStateManagerTest;

  void CheckInitialState();
  void UpdateCurrentEnvironment();
  void RequestCredentialSummary(const std::string& domain);
  void OnCredentialSummary(const std::string& domain,
                           skus::mojom::SkusResultPtr summary);
  void OnPrepareCredentialsPresentation(
      const std::string& domain,
      skus::mojom::SkusResultPtr credential_as_cookie);
  void OnGetSubscriberCredentialV12(base::Time expiration_time,
                                    const std::string& subscriber_credential,
                                    bool success);

  void ScheduleSubscriberCredentialRefresh();
  void RefreshSubscriberCredential();

#if !BUILDFLAG(IS_ANDROID)
  void UpdatePurchasedStateForSessionExpired(const std::string& env);
#endif

  const raw_ref<PrefService> local_prefs_;
  const raw_ref<SkusServiceClient> skus_client_;
  PurchasedStateChangedCallback purchased_state_changed_callback_;
  std::unique_ptr<CredentialStore> credential_store_;
  std::string current_environment_;
  std::optional<mojom::PurchasedInfo> purchased_state_;
  base::OneShotTimer subs_cred_refresh_timer_;
  base::WeakPtrFactory<PurchasedStateManager> weak_factory_{this};
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_PURCHASED_STATE_MANAGER_H_
