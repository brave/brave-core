/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_FACTORY_H_

#include "brave/components/brave_origin/brave_origin_service.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

namespace base {
template <typename T>
class NoDestructor;
}

class Profile;

namespace brave_origin {

class BraveOriginService;
enum class BraveOriginPolicyScope;

// Factory for BraveOriginService keyed service.
class BraveOriginServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static BraveOriginService* GetForProfile(Profile* profile);
  static BraveOriginServiceFactory* GetInstance();

  // Get policy key to pref name mappings for all BraveOrigin prefs
  static base::flat_map<std::string, std::string> GetPolicyKeyMappings();

  // Build policy definitions with all necessary dependencies
  static BraveOriginPolicyMap BuildBraveOriginPolicyDefnitions();

  // Initialize BraveOriginPrefs singleton - should be called early in startup
  static void InitializeBraveOriginManager();

  BraveOriginServiceFactory(const BraveOriginServiceFactory&) = delete;
  BraveOriginServiceFactory& operator=(const BraveOriginServiceFactory&) =
      delete;

 private:
  friend base::NoDestructor<BraveOriginServiceFactory>;

  // Static BraveOrigin-specific metadata for policy preferences.
  // This defines which preferences from kBraveSimplePolicyMap should have
  // BraveOrigin behavior and specifies their BraveOrigin-specific configuration
  // (default values, scope, UI visibility). Used only during initialization
  // to populate BraveOriginPolicyInfo structs.
  struct BraveOriginPrefMetadata {
    BraveOriginPrefMetadata(base::Value origin_default_value,
                            BraveOriginPolicyScope scope,
                            bool user_settable);
    ~BraveOriginPrefMetadata();

    // Copy operations are deleted because base::Value is not copyable
    BraveOriginPrefMetadata(const BraveOriginPrefMetadata&) = delete;
    BraveOriginPrefMetadata& operator=(const BraveOriginPrefMetadata&) = delete;

    BraveOriginPrefMetadata(BraveOriginPrefMetadata&&);
    BraveOriginPrefMetadata& operator=(BraveOriginPrefMetadata&&);

    base::Value origin_default_value;
    BraveOriginPolicyScope scope;
    bool user_settable;
  };

  BraveOriginServiceFactory();
  ~BraveOriginServiceFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
};

}  // namespace brave_origin

#endif  // BRAVE_BROWSER_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_FACTORY_H_
