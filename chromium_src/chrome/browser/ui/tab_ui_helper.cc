/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/tab_ui_helper.h"

#include <utility>

#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/containers/core/browser/prefs.h"
#include "chrome/browser/profiles/profile.h"

#include "src/chrome/browser/ui/tab_ui_helper.cc"

std::u16string TabUIHelper::GetTitle() const {
  if (auto* site_instance = web_contents()->GetSiteInstance()) {
    const auto& storage_partition_config =
        site_instance->GetStoragePartitionConfig();
    if (!storage_partition_config.is_default()) {
      auto container_name = base::UTF8ToUTF16(containers::GetContainerName(
          *Profile::FromBrowserContext(web_contents()->GetBrowserContext())
               ->GetPrefs(),
          storage_partition_config.partition_domain()));
      if (!container_name.empty()) {
        cached_container_name_ = std::move(container_name);
      }
    }
  }

  std::u16string title = TabUIHelper::GetTitle_ChromiumImpl();

  if (cached_container_name_.empty()) {
    return title;
  } else {
    return base::StrCat({u"[", cached_container_name_, u"] ", title});
  }
}
