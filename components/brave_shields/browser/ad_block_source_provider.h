/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SOURCE_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SOURCE_PROVIDER_H_

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"

using brave_component_updater::DATFileDataBuffer;

namespace brave_shields {

class AdBlockSourceProvider {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnDATLoaded(bool deserialize,
                             const DATFileDataBuffer& dat_buf) = 0;
  };

  AdBlockSourceProvider();
  virtual ~AdBlockSourceProvider();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void LoadDAT(Observer* observer);

 protected:
  virtual void LoadDATBuffer(
      base::OnceCallback<void(bool deserialize,
                              const DATFileDataBuffer& dat_buf)>) = 0;

  void OnLoad(AdBlockSourceProvider::Observer* observer,
              bool deserialize,
              const DATFileDataBuffer& dat_buf);
  void OnDATLoaded(bool deserialize, const DATFileDataBuffer& dat_buf);

 private:
  base::ObserverList<Observer> observers_;
  base::WeakPtrFactory<AdBlockSourceProvider> weak_factory_{this};
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SOURCE_PROVIDER_H_
