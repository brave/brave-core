/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/tabs/tab_renderer_data.h"

#define FromTabInModel FromTabInModel_ChromiumImpl
#include "src/chrome/browser/ui/tabs/tab_renderer_data.cc"
#undef FromTabInModel

#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service_factory.h"
#include "brave/components/constants/webui_url_constants.h"
#include "url/gurl.h"

TabRendererData TabRendererData::FromTabInModel(const TabStripModel* model,
                                                int index) {
  if (base::FeatureList::IsEnabled(tabs::features::kBraveSharedPinnedTabs)) {
    if (index < model->IndexOfFirstNonPinnedTab()) {
      auto* shared_pinned_tab_service =
          SharedPinnedTabServiceFactory::GetForProfile(model->profile());
      DCHECK(shared_pinned_tab_service);

      auto* contents = model->GetWebContentsAt(index);
      if (shared_pinned_tab_service->IsDummyContents(contents)) {
        if (const auto* data =
                shared_pinned_tab_service->GetTabRendererDataForDummyContents(
                    index, contents)) {
          return *data;
        }
      }
    }
  }

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
