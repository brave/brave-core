/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_H_

#include <memory>
#include <optional>
#include <string>

#include "base/containers/flat_map.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/brave_origin/brave_origin_policy_info.h"
#include "brave/components/brave_origin/buildflags/buildflags.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "build/build_config.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/remote.h"

class PrefService;

namespace policy {
class PolicyService;
}  // namespace policy

namespace brave_origin {

// This keyed service will maintain the definitions/mappings of policies to
// preferences.
//
// This is separate from BraveProfilePolicyProvider which handles the actual
// integration with Chromium's policy framework.
class BraveOriginService : public KeyedService {
 public:
  // Delegate for browser-layer actions the service cannot perform directly.
  class Delegate {
   public:
    virtual ~Delegate() = default;
    // Called once when a purchase is first detected, after credentials
    // have been verified.
    virtual void OpenOriginSettings() = 0;
    // Returns a pending remote for the SKU service.
    virtual mojo::PendingRemote<skus::mojom::SkusService> GetSkusService() = 0;
  };

  BraveOriginService(PrefService* local_state,
                     PrefService* profile_prefs,
                     std::string_view profile_id,
                     policy::PolicyService* profile_policy_service,
                     policy::PolicyService* browser_policy_service,
                     std::unique_ptr<Delegate> delegate);
  ~BraveOriginService() override;

  // KeyedService:
  void Shutdown() override;

  // Check if a policy is controlled by BraveOrigin
  bool IsPolicyControlledByBraveOrigin(std::string_view policy_key) const;

  // Update the BraveOrigin policy value
  bool SetPolicyValue(std::string_view policy_key, bool value);

  // Get the current value of a BraveOrigin policy
  std::optional<bool> GetPolicyValue(std::string_view policy_key) const;

  // Asynchronously check purchase state via SKU credential summary.
  // The callback receives true if the user has a valid Origin purchase.
  void CheckPurchaseState(base::OnceCallback<void(bool)> callback);

  // Returns the cached purchase state (synchronous).
  bool IsPurchased() const;

  // Returns true if any policy values have been changed since the service
  // was created (i.e., since browser startup). This indicates that a restart
  // is needed for the changes to fully take effect.
  bool NeedsRestart() const;

#if BUILDFLAG(IS_LINUX)
  // Accept the Linux free tier: sets the kOriginFreeTierAccepted pref and
  // marks the policy manager as purchased so policies take effect.
  void AcceptFreeTier();

  // Returns true if the user has accepted the Linux free tier.
  bool IsFreeTierAccepted() const;
#endif

 protected:
  // Local state and profile preferences this state is associated with
  raw_ptr<PrefService> local_state_;
  raw_ptr<PrefService> profile_prefs_;

  // The profile_id is a calculated hash which will be used to look up
  // the policy values for a particular profile.
  std::string profile_id_;
  raw_ptr<policy::PolicyService> profile_policy_service_;
  raw_ptr<policy::PolicyService> browser_policy_service_;

 private:
  void OnCredentialSummary(base::OnceCallback<void(bool)> callback,
                           skus::mojom::SkusResultPtr summary);
  void OnSkusStateChanged();
  bool EnsureSkusConnected();

  mojo::Remote<skus::mojom::SkusService> skus_service_;
  std::string origin_sku_domain_;

  // Whether Origin policies were being enforced in the previous session.
  // Read from kOriginPoliciesWereEnforced pref at construction.
  bool startup_was_enforcing_;

  // Snapshot of policy values at construction time, used to detect
  // settings changes that require a restart.
  base::flat_map<std::string, bool> startup_browser_policies_;
  base::flat_map<std::string, bool> startup_profile_policies_;

#if !BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
  // Whether OpenOriginSettings() has already been called this session.
  // Only used in the upgrade case; branded builds open the dialog at startup.
  bool did_open_origin_settings_ = false;
#endif

  // Browser-layer delegate for actions like opening the settings page.
  std::unique_ptr<Delegate> delegate_;

  // Re-checks purchase state when SKU credentials change (e.g. after
  // the user completes a purchase on account.brave.com).
  PrefChangeRegistrar skus_pref_registrar_;

  base::WeakPtrFactory<BraveOriginService> weak_ptr_factory_{this};
};

}  // namespace brave_origin

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_H_
