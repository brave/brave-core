// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_RESOURCE_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_RESOURCE_PROVIDER_H_

#include <string>

#include "base/functional/callback.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_shields/core/browser/adblock/rs/src/lib.rs.h"
#include "third_party/rust/cxx/v1/cxx.h"

using brave_component_updater::DATFileDataBuffer;

namespace brave_shields {

// Type alias for the Brave core resource storage box
using BraveResourceStorageBox = rust::Box<adblock::BraveCoreResourceStorage>;

// Interface for any source that can load resource replacements into an adblock
// engine.
class AdBlockResourceProvider {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnResourcesLoaded(BraveResourceStorageBox) = 0;
  };

  AdBlockResourceProvider();
  virtual ~AdBlockResourceProvider();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  virtual void LoadResources(
      base::OnceCallback<void(BraveResourceStorageBox)>) = 0;

 protected:
  void NotifyResourcesLoaded(BraveResourceStorageBox);

 private:
  base::ObserverList<Observer> observers_;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_RESOURCE_PROVIDER_H_
