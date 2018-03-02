/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_RENDERER_HOST_CHROME_NAVIGATION_UI_DATA_H_
#define BRAVE_BROWSER_RENDERER_HOST_CHROME_NAVIGATION_UI_DATA_H_

#include "chrome/browser/renderer_host/chrome_navigation_ui_data.h"
#include "url/gurl.h"


class BraveNavigationUIData : public ChromeNavigationUIData {
 public:
  BraveNavigationUIData();
  explicit BraveNavigationUIData(content::NavigationHandle* navigation_handle);
  ~BraveNavigationUIData() override;

  // Creates a new BraveNavigationUIData that is a deep copy of the original.
  // Any changes to the original after the clone is created will not be
  // reflected in the clone.  All owned data members are deep copied.
  std::unique_ptr<content::NavigationUIData> Clone() const override;
  GURL GetURL() const;

 private:
  GURL url_;

  DISALLOW_COPY_AND_ASSIGN(BraveNavigationUIData);
};

#endif  // BRAVE_BROWSER_RENDERER_HOST_CHROME_NAVIGATION_UI_DATA_H_
