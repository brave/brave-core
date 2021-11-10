/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SOURCE_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SOURCE_PROVIDER_H_

#include "base/callback.h"
#include "base/observer_list.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

using brave_component_updater::DATFileDataBuffer;

namespace brave_shields {

class SourceProvider {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnNewDATAvailable(const DATFileDataBuffer& dat_buf) = 0;
    virtual void OnNewListSourceAvailable(
        const DATFileDataBuffer& list_source) = 0;
  };

  SourceProvider();
  virtual ~SourceProvider();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  virtual void Load(
      base::OnceCallback<void(bool deserialize,
                              const DATFileDataBuffer& dat_buf)>) = 0;

 protected:
  void ProvideNewDAT(const DATFileDataBuffer& dat_buf);
  void ProvideNewListSource(const DATFileDataBuffer& list_source);

 private:
  base::ObserverList<Observer> observers_;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SOURCE_PROVIDER_H_
