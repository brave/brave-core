/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SUBSCRIPTION_SOURCE_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SUBSCRIPTION_SOURCE_PROVIDER_H_

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_shields/browser/ad_block_source_provider.h"

using brave_component_updater::DATFileDataBuffer;

class PrefService;

namespace brave_shields {

class AdBlockSubscriptionSourceProvider : public AdBlockSourceProvider {
 public:
  AdBlockSubscriptionSourceProvider(PrefService* local_state,
                                    base::FilePath list_file);
  AdBlockSubscriptionSourceProvider(const AdBlockSubscriptionSourceProvider&) =
      delete;
  AdBlockSubscriptionSourceProvider& operator=(
      const AdBlockSubscriptionSourceProvider&) = delete;
  ~AdBlockSubscriptionSourceProvider() override;

  void ReloadListFromDisk();

  void LoadDATBuffer(
      base::OnceCallback<void(bool deserialize,
                              const DATFileDataBuffer& dat_buf)>) override;

 private:
  base::FilePath list_file_;

  base::WeakPtrFactory<AdBlockSubscriptionSourceProvider> weak_factory_{this};
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SUBSCRIPTION_SOURCE_PROVIDER_H_
