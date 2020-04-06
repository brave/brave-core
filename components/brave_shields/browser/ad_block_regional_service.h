/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_SERVICE_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "brave/components/brave_shields/browser/ad_block_base_service.h"

class AdBlockServiceTest;

namespace brave_shields {

// The brave shields service in charge of ad-block checking and init
// for a specific region.
class AdBlockRegionalService : public AdBlockBaseService {
 public:
  explicit AdBlockRegionalService(
      const std::string& uuid,
      brave_component_updater::BraveComponent::Delegate* delegate);
  ~AdBlockRegionalService() override;

  std::string GetUUID() const { return uuid_; }
  std::string GetTitle() const { return title_; }

 protected:
  bool Init() override;
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

 private:
  friend class ::AdBlockServiceTest;
  static std::string g_ad_block_regional_component_id_;
  static std::string g_ad_block_regional_component_base64_public_key_;
  static std::string g_ad_block_regional_dat_file_version_;
  static void SetComponentIdAndBase64PublicKeyForTest(
      const std::string& component_id,
      const std::string& component_base64_public_key);

  std::string uuid_;
  std::string title_;

  DISALLOW_COPY_AND_ASSIGN(AdBlockRegionalService);
};

// Creates the AdBlockRegionalService
std::unique_ptr<AdBlockRegionalService> AdBlockRegionalServiceFactory(
    const std::string& uuid,
    brave_component_updater::BraveComponent::Delegate* delegate);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_SERVICE_H_
