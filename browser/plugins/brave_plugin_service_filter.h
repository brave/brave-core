/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PLUGINS_BRAVE_PLUGIN_SERVICE_FILTER_H_
#define BRAVE_BROWSER_PLUGINS_BRAVE_PLUGIN_SERVICE_FILTER_H_

#include "chrome/browser/plugins/chrome_plugin_service_filter.h"

class BravePluginServiceFilter : public ChromePluginServiceFilter {
  public:
    static BravePluginServiceFilter* GetInstance();

  private:
    friend struct base::DefaultSingletonTraits<BravePluginServiceFilter>;

    BravePluginServiceFilter();
    ~BravePluginServiceFilter() override;

    // content::NotificationObserver implementation:
    void Observe(int type,
                 const content::NotificationSource& source,
                 const content::NotificationDetails& details) override;

    DISALLOW_COPY_AND_ASSIGN(BravePluginServiceFilter);
};

#endif  // BRAVE_BROWSER_PLUGINS_BRAVE_PLUGIN_SERVICE_FILTER_H_
