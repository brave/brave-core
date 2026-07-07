/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_PURCHASED_STATE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_PURCHASED_STATE_MANAGER_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "brave/components/skus/common/skus_sdk.mojom-forward.h"

class PrefService;

namespace brave_vpn::v2 {

class CredentialStore;

// PurchasedStateManager owns the purchased-state value and the current
// environment, and runs the credential chain that produces that state.
//
// Purchase state loading rules:
//  1. A load for the CURRENT environment is visible: it updates the purchased
//  state and notifies the service callback.
//  2. A load for a DIFFERENT environment runs silently and touches no
//  persistent state until authorization succeeds (a valid SKUS credential is
//  obtained for it). That success is the commit point: the credential is cached
//  and the environment becomes current. Note the commit is destructive - the
//  credential slot is shared, so committing evicts the previous environment's
//  credential; failing after that point cannot roll back to the previous
//  PURCHASED state.
//  3. At most one load is in flight. Starting a new load cancels the previous
//  one: its responses are dropped by a sequence check. A repeated load for the
//  environment already in flight is a duplicate and is ignored.
//  4. A silent load that terminates without committing will not strand the
//  visible state: if it was left unsettled (LOADING), the current environment
//  is reloaded.
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
  friend class PurchasedStateManagerTest;

  void CheckInitialState();

  void BeginLoad(const std::string& env);
  void FinishLoad(const std::string& env,
                  mojom::PurchasedState state,
                  std::optional<std::string> description);
  void CancelPendingLoad();

  void RequestCredentialSummary(const std::string& domain);
  void OnCredentialSummary(uint64_t sequence,
                           const std::string& domain,
                           skus::mojom::SkusResultPtr summary);
  void RunPurchasedStateCallback(mojom::PurchasedState state,
                                 std::optional<std::string> description);
  void ScheduleSubscriberCredentialRefresh();

  const raw_ref<PrefService> local_prefs_;
  PurchasedStateChangedCallback purchased_state_changed_callback_;
  std::unique_ptr<CredentialStore> credential_store_;
  std::optional<mojom::PurchasedInfo> purchased_state_;
  std::string loading_environment_;
  uint64_t loading_sequence_ = 0;
  base::WeakPtrFactory<PurchasedStateManager> weak_factory_{this};
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_PURCHASED_STATE_MANAGER_H_
