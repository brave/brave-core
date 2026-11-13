/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/sponsored_content/test/fake_sponsored_content_service.h"

#include "base/files/file_path.h"

namespace sponsored_content::test {

FakeSponsoredContentService::FakeSponsoredContentService()
    : SponsoredContentService(/*variations_service=*/nullptr,
                              /*component_update_service=*/nullptr) {}

FakeSponsoredContentService::~FakeSponsoredContentService() = default;

void FakeSponsoredContentService::RegisterSponsoredContentComponent() {
  ++register_call_count_;
}

void FakeSponsoredContentService::UnregisterSponsoredContentComponent() {
  ++unregister_call_count_;
}

void FakeSponsoredContentService::NotifySponsoredNewTabTakeoverContent(
    const base::FilePath& installed_dir,
    const base::DictValue& dict) {
  last_sponsored_new_tab_takeover_content_ = dict.Clone();
  SetInstalledDirForTesting(installed_dir);
  HandleSponsoredNewTabTakeoverContentCallback(dict.Clone());
}

void FakeSponsoredContentService::NotifySponsoredSiteContent(
    const base::FilePath& installed_dir,
    const base::DictValue& dict) {
  last_sponsored_site_content_ = dict.Clone();
  SetInstalledDirForTesting(installed_dir);
  HandleSponsoredSiteContentCallback(dict.Clone());
}

void FakeSponsoredContentService::NotifySponsoredConversionConfigContent(
    const base::FilePath& installed_dir,
    const base::DictValue& dict) {
  last_sponsored_conversion_config_content_ = dict.Clone();
  SetInstalledDirForTesting(installed_dir);
  HandleSponsoredConversionConfigContentCallback(dict.Clone());
}

void FakeSponsoredContentService::LoadAndNotifyContent(
    const base::FilePath& installed_dir) {
  SetInstalledDirForTesting(installed_dir);

  if (last_sponsored_new_tab_takeover_content_) {
    HandleSponsoredNewTabTakeoverContentCallback(
        last_sponsored_new_tab_takeover_content_->Clone());
  }
  if (last_sponsored_site_content_) {
    HandleSponsoredSiteContentCallback(last_sponsored_site_content_->Clone());
  }
  if (last_sponsored_conversion_config_content_) {
    HandleSponsoredConversionConfigContentCallback(
        last_sponsored_conversion_config_content_->Clone());
  }
}

}  // namespace sponsored_content::test
