/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_FACTORY_H_

#include "brave/components/brave_origin/brave_origin_policy_info.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

namespace base {
template <typename T>
class NoDestructor;
}

class Profile;

namespace brave_origin {

class BraveOriginService;

// Factory for BraveOriginService keyed service.
class BraveOriginServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static BraveOriginService* GetForProfile(Profile* profile);
  static BraveOriginServiceFactory* GetInstance();

  BraveOriginServiceFactory(const BraveOriginServiceFactory&) = delete;
  BraveOriginServiceFactory& operator=(const BraveOriginServiceFactory&) =
      delete;

  // Build browser-level policy definitions.
  // This is done in this layer because of all the dependencies needed
  // to gather this information.
  static BraveOriginPolicyMap GetBrowserPolicyDefinitions();

  // Build profile-level policy definitions for a specific profile.
  static BraveOriginPolicyMap GetProfilePolicyDefinitions();

  // Static BraveOrigin-specific metadata for policy preferences.
  // This defines which preferences from kBraveSimplePolicyMap should have
  // BraveOrigin behavior and specifies their BraveOrigin-specific configuration
  // (default values, scope, UI visibility). Used only during initialization
  // to populate BraveOriginPrefInfo structs.
  struct BraveOriginPrefMetadata {
    constexpr BraveOriginPrefMetadata(bool origin_default_value,
                                      bool user_settable)
        : origin_default_value(origin_default_value),
          user_settable(user_settable) {}
    constexpr ~BraveOriginPrefMetadata() = default;

    // Allow copy operations for MakeFixedFlatMap
    constexpr BraveOriginPrefMetadata(const BraveOriginPrefMetadata&) = default;
    constexpr BraveOriginPrefMetadata& operator=(
        const BraveOriginPrefMetadata&) = default;

    constexpr BraveOriginPrefMetadata(BraveOriginPrefMetadata&&) = default;
    constexpr BraveOriginPrefMetadata& operator=(BraveOriginPrefMetadata&&) =
        default;

    bool origin_default_value;
    bool user_settable;
  };

 private:
  friend base::NoDestructor<BraveOriginServiceFactory>;

  BraveOriginServiceFactory();
  ~BraveOriginServiceFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
};

}  // namespace brave_origin

#endif  // BRAVE_BROWSER_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_FACTORY_H_
