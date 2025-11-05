// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_NEW_TAB_PAGE_INITIALIZER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_NEW_TAB_PAGE_INITIALIZER_H_

#include <string_view>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"

namespace content {
class WebUI;
class WebUIDataSource;
}  // namespace content

namespace regional_capabilities {
class RegionalCapabilitiesService;
}

class Profile;

namespace brave_new_tab_page_refresh {

// Returns the hostname of the default search engine used on the NTP based upon
// the user's region.
std::string_view GetSearchDefaultHost(
    regional_capabilities::RegionalCapabilitiesService* regional_capabilities);

// Responsible for initialization of the Brave NTP WebUI, including strings,
// resources, data sources, and CSP.
class NewTabPageInitializer {
 public:
  explicit NewTabPageInitializer(content::WebUI& web_ui);
  ~NewTabPageInitializer();

  NewTabPageInitializer(const NewTabPageInitializer&) = delete;
  NewTabPageInitializer& operator=(const NewTabPageInitializer&) = delete;

  void Initialize();

 private:
  Profile* GetProfile();

  void AddCSPOverrides();
  void AddLoadTimeValues();
  void AddStrings();
  void AddPluralStrings();
  void AddResourcePaths();
  void AddFaviconDataSource();
  void AddCustomImageDataSource();
  void AddSanitizedImageDataSource();
  void MaybeMigrateHideAllWidgetsPref();

  raw_ref<content::WebUI> web_ui_;
  raw_ptr<content::WebUIDataSource> source_ = nullptr;
};

}  // namespace brave_new_tab_page_refresh

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_NEW_TAB_PAGE_INITIALIZER_H_
