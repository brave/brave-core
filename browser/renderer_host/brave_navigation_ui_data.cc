/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/renderer_host/brave_navigation_ui_data.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"

BraveNavigationUIData::BraveNavigationUIData()
    : ChromeNavigationUIData(),
      tor_profile_service_(nullptr) {}

BraveNavigationUIData::BraveNavigationUIData(
    NavigationHandle* navigation_handle)
    : ChromeNavigationUIData(navigation_handle),
      tor_profile_service_(nullptr) {}

BraveNavigationUIData::~BraveNavigationUIData() {}

// static
std::unique_ptr<ChromeNavigationUIData>
BraveNavigationUIData::CreateForMainFrameNavigation(
    content::WebContents* web_contents,
    WindowOpenDisposition disposition) {
  auto navigation_ui_data =
    ChromeNavigationUIData::CreateForMainFrameNavigation(
        web_contents, disposition);
  BraveNavigationUIData* ui_data =
    static_cast<BraveNavigationUIData*>(navigation_ui_data.get());

  Profile* profile = Profile::FromBrowserContext(web_contents->GetBrowserContext());
  TorProfileServiceFactory::SetTorNavigationUIData(
      profile,
      ui_data);

  return navigation_ui_data;
}

std::unique_ptr<content::NavigationUIData> BraveNavigationUIData::Clone()
    const {
      content::NavigationUIData* chrome_copy =
    (ChromeNavigationUIData::Clone().release());
  BraveNavigationUIData* copy =
    static_cast<BraveNavigationUIData*>(chrome_copy);

  copy->tor_profile_service_ = tor_profile_service_;

  return base::WrapUnique(copy);
}

void BraveNavigationUIData::SetTorProfileService(
    TorProfileService* tor_profile_service) {
  tor_profile_service_ = tor_profile_service;
}

TorProfileService* BraveNavigationUIData::GetTorProfileService() const {
  return tor_profile_service_;
}
