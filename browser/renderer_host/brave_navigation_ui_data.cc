/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/renderer_host/brave_navigation_ui_data.h"

#include "base/memory/ptr_util.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_ui_data.h"


BraveNavigationUIData::BraveNavigationUIData() {}

BraveNavigationUIData::BraveNavigationUIData(
    content::NavigationHandle* navigation_handle) :
    ChromeNavigationUIData(navigation_handle) {
  auto* web_contents = navigation_handle->GetWebContents();
  url_ = web_contents->GetURL();
}

BraveNavigationUIData::~BraveNavigationUIData() {}

std::unique_ptr<content::NavigationUIData> BraveNavigationUIData::Clone()
    const {

  std::unique_ptr<BraveNavigationUIData> copy =
      base::MakeUnique<BraveNavigationUIData>();

  //////////////////////////////////////////////////
  // This is a copy of ChromeNavigationUIData::Clone
#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (extension_data_)
    copy->SetExtensionNavigationUIData(extension_data_->DeepCopy());
#endif

#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
  if (offline_page_data_)
    copy->SetOfflinePageNavigationUIData(offline_page_data_->DeepCopy());
#endif

  copy->prerender_mode_ = prerender_mode_;
  copy->prerender_histogram_prefix_ = prerender_histogram_prefix_;
  // End copy of ChromeNavigationUIData::Clone
  ////////////////////////////////////////////

  copy->url_ = url_;

  return std::move(copy);
}

GURL BraveNavigationUIData::GetURL() const {
  return url_;
}
