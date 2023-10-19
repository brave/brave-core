/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_FILTERS_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_FILTERS_PROVIDER_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"

using brave_component_updater::DATFileDataBuffer;

namespace brave_shields {

// Interface for any source that can load filters or serialized filter data
// into an adblock engine.
class AdBlockFiltersProvider {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnChanged(bool is_for_default_engine) = 0;
  };

  explicit AdBlockFiltersProvider(bool engine_is_default);
  // Used by AdblockFiltersProviderManager
  AdBlockFiltersProvider();
  AdBlockFiltersProvider(const AdBlockFiltersProvider&) = delete;
  AdBlockFiltersProvider& operator=(const AdBlockFiltersProvider&) = delete;
  virtual ~AdBlockFiltersProvider();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void LoadDAT(base::OnceCallback<void(bool deserialize,
                                       const DATFileDataBuffer& dat_buf)>);

  base::WeakPtr<AdBlockFiltersProvider> AsWeakPtr();

  virtual std::string GetNameForDebugging() = 0;

 protected:
  bool engine_is_default_;

  virtual void LoadDATBuffer(
      base::OnceCallback<void(bool deserialize,
                              const DATFileDataBuffer& dat_buf)>) = 0;
  void NotifyObservers(bool is_for_default_engine);

 private:
  base::ObserverList<Observer> observers_;
  base::WeakPtrFactory<AdBlockFiltersProvider> weak_factory_{this};
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_FILTERS_PROVIDER_H_
