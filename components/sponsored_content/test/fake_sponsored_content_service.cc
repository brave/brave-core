/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/sponsored_content/test/fake_sponsored_content_service.h"

#include <optional>

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
    base::optional_ref<const base::DictValue> dict) {
  last_sponsored_new_tab_takeover_content_ =
      dict ? std::make_optional(dict->Clone()) : std::nullopt;
  SetInstalledDirForTesting(installed_dir);
  HandleSponsoredNewTabTakeoverContentCallback(
      installed_dir,
      last_sponsored_new_tab_takeover_content_
          ? std::make_optional(
                last_sponsored_new_tab_takeover_content_->Clone())
          : std::nullopt);
}

void FakeSponsoredContentService::NotifySponsoredSiteContent(
    const base::FilePath& installed_dir,
    const base::DictValue& dict) {
  last_sponsored_site_content_ = dict.Clone();
  SetInstalledDirForTesting(installed_dir);
  HandleSponsoredSiteContentCallback(installed_dir, dict.Clone());
}

void FakeSponsoredContentService::NotifySponsoredConversionConfigContent(
    const base::FilePath& installed_dir,
    const base::DictValue& dict) {
  last_sponsored_conversion_config_content_ = dict.Clone();
  SetInstalledDirForTesting(installed_dir);
  HandleSponsoredConversionConfigContentCallback(installed_dir, dict.Clone());
}

void FakeSponsoredContentService::LoadAndNotifyContent(
    const base::FilePath& installed_dir) {
  SetInstalledDirForTesting(installed_dir);

  if (last_sponsored_new_tab_takeover_content_) {
    HandleSponsoredNewTabTakeoverContentCallback(
        installed_dir, last_sponsored_new_tab_takeover_content_->Clone());
  }
  if (last_sponsored_site_content_) {
    HandleSponsoredSiteContentCallback(installed_dir,
                                       last_sponsored_site_content_->Clone());
  }
  if (last_sponsored_conversion_config_content_) {
    HandleSponsoredConversionConfigContentCallback(
        installed_dir, last_sponsored_conversion_config_content_->Clone());
  }
}

}  // namespace sponsored_content::test
