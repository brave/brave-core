/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/tabs/tab_renderer_data.h"

#include "base/check.h"
#include "base/feature_list.h"

#define FromTabInModel FromTabInModel_ChromiumImpl
#include <chrome/browser/ui/tabs/tab_renderer_data.cc>
#undef FromTabInModel

#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service_factory.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/resource_coordinator/tab_load_tracker.h"
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

  // Show which tabs are unloaded.
  if (!data.should_show_discard_status) {
    content::WebContents* const contents = model->GetWebContentsAt(index);
    using resource_coordinator::TabLoadTracker;
    const auto loading_state = TabLoadTracker::Get()->GetLoadingState(contents);
    if (loading_state == TabLoadTracker::LoadingState::UNLOADED) {
      data.should_show_discard_status = true;
    }
  }

  if (base::FeatureList::IsEnabled(tabs::features::kBraveRenamingTabs)) {
    tabs::TabInterface* const tab = model->GetTabAtIndex(index);
    CHECK(tab);

    tabs::TabFeatures* const features = tab->GetTabFeatures();
    CHECK(features);

    TabUIHelper* const tab_ui_helper = features->tab_ui_helper();
    CHECK(tab_ui_helper);
    data.is_custom_title = tab_ui_helper->has_custom_title();
  }

  if (base::FeatureList::IsEnabled(tabs::features::kBraveEmojiTabFavicon)) {
    // If a custom emoji favicon is set, override the favicon image.
    tabs::TabInterface* const tab_for_favicon = model->GetTabAtIndex(index);
    if (tab_for_favicon) {
      tabs::TabFeatures* const features_for_favicon =
          tab_for_favicon->GetTabFeatures();
      if (features_for_favicon) {
        TabUIHelper* const tab_ui_helper_for_favicon =
            features_for_favicon->tab_ui_helper();
        if (tab_ui_helper_for_favicon &&
            tab_ui_helper_for_favicon->has_custom_emoji_favicon()) {
          data.favicon = tab_ui_helper_for_favicon->GetEmojiFaviconImage();
          data.is_monochrome_favicon = false;
          data.should_themify_favicon = false;
        }
      }
    }
  }

  return data;
}
