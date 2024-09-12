/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_GREASELION_BROWSER_GREASELION_SERVICE_H_
#define BRAVE_COMPONENTS_GREASELION_BROWSER_GREASELION_SERVICE_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "components/keyed_service/core/keyed_service.h"
#include "extensions/browser/extension_registry_observer.h"
#include "extensions/common/extension_id.h"

class GreaselionServiceTest;

namespace base {
class Version;
}

namespace greaselion {

enum GreaselionFeature {
  FIRST_FEATURE = 0,
  REWARDS = FIRST_FEATURE,
  AUTO_CONTRIBUTION,
  ADS,
  SUPPORTS_MINIMUM_BRAVE_VERSION,
  LAST_FEATURE
};

typedef std::map<GreaselionFeature, bool> GreaselionFeatures;

class GreaselionService : public KeyedService,
                          public extensions::ExtensionRegistryObserver {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;
    virtual bool IsEnabled() const = 0;
    virtual void AddExtension(extensions::Extension* extension) = 0;
    virtual void UnloadExtension(const std::string& extension_id) = 0;
  };

  GreaselionService() = default;
  GreaselionService(const GreaselionService&) = delete;
  GreaselionService& operator=(const GreaselionService&) = delete;

  // KeyedService
  void Shutdown() override {}

  virtual void SetFeatureEnabled(GreaselionFeature feature, bool enabled) = 0;
  virtual void UpdateInstalledExtensions() = 0;
  virtual bool IsGreaselionExtension(const std::string& id) = 0;
  virtual std::vector<extensions::ExtensionId> GetExtensionIdsForTesting() = 0;
  virtual bool update_in_progress() = 0;
  virtual bool rules_ready() = 0;

  // implementation of our own observers
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnRulesReady(GreaselionService* greaselion_service) {}
    virtual void OnExtensionsReady(GreaselionService* greaselion_service,
                                   bool success) {}
  };
  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;

 private:
  friend class ::GreaselionServiceTest;
  virtual void SetBrowserVersionForTesting(const base::Version& version) = 0;
};

}  // namespace greaselion

#endif  // BRAVE_COMPONENTS_GREASELION_BROWSER_GREASELION_SERVICE_H_
