/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_SOURCE_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_SOURCE_PROVIDER_H_

#include <string>

#include "base/callback.h"
#include "base/observer_list.h"
#include "brave/components/adblock_rust_ffi/src/wrapper.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_shields/browser/ad_block_source_provider.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

using brave_component_updater::DATFileDataBuffer;

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace base {
class FilePath;
}

class AdBlockServiceTest;

namespace brave_shields {

class AdBlockRegionalSourceProvider : public SourceProvider {
 public:
  AdBlockRegionalSourceProvider(component_updater::ComponentUpdateService* cus,
                                const adblock::FilterList& catalog_entry);
  ~AdBlockRegionalSourceProvider() override;

  void Load(base::OnceCallback<
            void(bool deserialize, const DATFileDataBuffer& dat_buf)>) override;

 private:
  friend class ::AdBlockServiceTest;

  void OnComponentReady(const base::FilePath&);

  base::FilePath component_path_;
  std::string uuid_;
  std::string component_id_;
  component_updater::ComponentUpdateService* component_updater_service_;

  base::WeakPtrFactory<AdBlockRegionalSourceProvider> weak_factory_{this};

  AdBlockRegionalSourceProvider(const AdBlockRegionalSourceProvider&) = delete;
  AdBlockRegionalSourceProvider& operator=(
      const AdBlockRegionalSourceProvider&) = delete;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_SOURCE_PROVIDER_H_
