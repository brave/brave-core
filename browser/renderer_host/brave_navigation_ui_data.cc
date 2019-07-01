/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/renderer_host/brave_navigation_ui_data.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"

BraveNavigationUIData::BraveNavigationUIData()
    : ChromeNavigationUIData(), is_tor_(false) {}

BraveNavigationUIData::BraveNavigationUIData(
    NavigationHandle* navigation_handle)
    : ChromeNavigationUIData(navigation_handle),
      is_tor_(false) {}

BraveNavigationUIData::~BraveNavigationUIData() {}

// static
std::unique_ptr<ChromeNavigationUIData>
BraveNavigationUIData::CreateForMainFrameNavigation(
    content::WebContents* web_contents,
    WindowOpenDisposition disposition,
    int64_t data_reduction_proxy_page_id) {
  auto navigation_ui_data =
      ChromeNavigationUIData::CreateForMainFrameNavigation(
          web_contents, disposition, data_reduction_proxy_page_id);
  BraveNavigationUIData* ui_data =
      static_cast<BraveNavigationUIData*>(navigation_ui_data.get());

  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  if (profile && profile->IsTorProfile())
    ui_data->SetTor(true);

  return navigation_ui_data;
}

std::unique_ptr<content::NavigationUIData> BraveNavigationUIData::Clone()
    const {
  content::NavigationUIData* chrome_copy =
      (ChromeNavigationUIData::Clone().release());
  BraveNavigationUIData* copy =
      static_cast<BraveNavigationUIData*>(chrome_copy);

  copy->is_tor_ = is_tor_;

  return base::WrapUnique(copy);
}

void BraveNavigationUIData::SetTor(bool is_tor) {
  is_tor_ = is_tor;
}

bool BraveNavigationUIData::IsTor() const {
  return is_tor_;
}
