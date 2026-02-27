/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_H_

#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/brave_origin/brave_origin_policy_info.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
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
  using SkusServiceGetter =
      base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>;

  BraveOriginService(PrefService* local_state,
                     PrefService* profile_prefs,
                     std::string_view profile_id,
                     policy::PolicyService* profile_policy_service,
                     policy::PolicyService* browser_policy_service,
                     SkusServiceGetter skus_service_getter);
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
  bool EnsureSkusConnected();

  SkusServiceGetter skus_service_getter_;
  mojo::Remote<skus::mojom::SkusService> skus_service_;
  std::string origin_sku_domain_;

  base::WeakPtrFactory<BraveOriginService> weak_ptr_factory_{this};
};

}  // namespace brave_origin

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_H_
