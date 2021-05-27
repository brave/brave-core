/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/tabs/tab_renderer_data.h"

#define FromTabInModel FromTabInModel_ChromiumImpl
#include "../../../../../../chrome/browser/ui/tabs/tab_renderer_data.cc"
#undef FromTabInModel

#include "brave/common/webui_url_constants.h"
#include "url/gurl.h"

TabRendererData TabRendererData::FromTabInModel(TabStripModel* model,
                                                int index) {
  auto data = FromTabInModel_ChromiumImpl(model, index);
  if (data.should_themify_favicon) {
    content::WebContents* const contents = model->GetWebContentsAt(index);
    const GURL& url = contents->GetVisibleURL();
    if (url.SchemeIs(content::kChromeUIScheme) &&
        (url.host_piece() == kWelcomeHost ||
         url.host_piece() == kRewardsPageHost)) {
      data.should_themify_favicon = false;
    }
  }
  return data;
}
