/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_GREASELION_BROWSER_GREASELION_SERVICE_H_
#define BRAVE_COMPONENTS_GREASELION_BROWSER_GREASELION_SERVICE_H_

#include <map>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/observer_list.h"
#include "components/keyed_service/core/keyed_service.h"
#include "extensions/browser/extension_registry_observer.h"
#include "extensions/common/extension_id.h"
#include "url/gurl.h"

namespace greaselion {

enum GreaselionFeature {
  FIRST_FEATURE = 0,
  TWITTER_TIPS = FIRST_FEATURE,
  REDDIT_TIPS,
  GITHUB_TIPS,
  AUTO_CONTRIBUTION,
  LAST_FEATURE
};

typedef std::map<GreaselionFeature, bool> GreaselionFeatures;

class GreaselionService : public KeyedService,
                          public extensions::ExtensionRegistryObserver {
 public:
  GreaselionService() = default;

  virtual void SetFeatureEnabled(GreaselionFeature feature, bool enabled) = 0;
  virtual void UpdateInstalledExtensions() = 0;
  virtual bool IsGreaselionExtension(const std::string& id) = 0;
  virtual std::vector<extensions::ExtensionId> GetExtensionIdsForTesting() = 0;
  virtual bool ready() = 0;

  // implementation of our own observers
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnExtensionsReady(GreaselionService* greaselion_service,
                                   bool success) = 0;
  };
  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(GreaselionService);
};

}  // namespace greaselion

#endif  // BRAVE_COMPONENTS_GREASELION_BROWSER_GREASELION_SERVICE_H_
