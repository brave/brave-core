/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_RENDERER_HOST_BRAVE_NAVIGATION_UI_DATA_H_
#define BRAVE_BROWSER_RENDERER_HOST_BRAVE_NAVIGATION_UI_DATA_H_

#include <memory>

#include "brave/browser/tor/tor_profile_service.h"
#include "chrome/browser/renderer_host/chrome_navigation_ui_data.h"

using content::NavigationHandle;
using tor::TorProfileService;

enum class WindowOpenDisposition;

class BraveNavigationUIData : public ChromeNavigationUIData {
 public:
  BraveNavigationUIData();
  explicit BraveNavigationUIData(NavigationHandle* navigation_handle);
  ~BraveNavigationUIData() override;

  static std::unique_ptr<ChromeNavigationUIData> CreateForMainFrameNavigation(
      content::WebContents* web_contents,
      WindowOpenDisposition disposition,
      int64_t data_reduction_proxy_page_id);

  std::unique_ptr<content::NavigationUIData> Clone() override;

  void SetTorProfileService(TorProfileService*);
  TorProfileService* GetTorProfileService() const;

 private:
  TorProfileService* tor_profile_service_;

  DISALLOW_COPY_AND_ASSIGN(BraveNavigationUIData);
};

#endif  // BRAVE_BROWSER_RENDERER_HOST_BRAVE_NAVIGATION_UI_DATA_H_
