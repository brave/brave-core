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

#include "brave/browser/ui/tabs/shared_pinned_tab_service.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service_factory.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/resource_coordinator/tab_load_tracker.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/common/webui_url_constants.h"
#include "components/grit/brave_components_scaled_resources.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/color_utils.h"
#include "url/gurl.h"

namespace {

// Returns the appropriate favicon for the NTP based on the current theme.
ui::ImageModel GetThemedNTPFavicon(content::WebContents* contents) {
  const auto& color_provider = contents->GetColorProvider();
  const SkColor background_color =
      color_provider.GetColor(kColorTabBackgroundActiveFrameActive);
  const bool is_dark = color_utils::IsDark(background_color);
  const int resource_id =
      is_dark ? IDR_FAVICON_NTP_DARK : IDR_FAVICON_NTP_LIGHT;
  return ui::ImageModel::FromImage(
      ui::ResourceBundle::GetSharedInstance().GetImageNamed(resource_id));
}

}  // namespace

TabRendererData TabRendererData::FromTabInModel(const TabStripModel* model,
                                                int index) {
  if (base::FeatureList::IsEnabled(tabs::kBraveSharedPinnedTabs)) {
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

  content::WebContents* const contents = model->GetWebContentsAt(index);
  const GURL& url = contents->GetVisibleURL();

  // Override favicon theming for some WebUIs.
  if (url.SchemeIs(content::kChromeUIScheme)) {
    if (url.host() == chrome::kChromeUINewTabHost) {
      data.favicon = GetThemedNTPFavicon(contents);
      data.should_themify_favicon = false;
    } else if (url.host() == kWelcomeHost || url.host() == kRewardsPageHost) {
      data.should_themify_favicon = false;
    }
  }

  // Show which tabs are unloaded.
  if (!data.should_show_discard_status) {
    using resource_coordinator::TabLoadTracker;
    const auto loading_state = TabLoadTracker::Get()->GetLoadingState(contents);
    if (loading_state == TabLoadTracker::LoadingState::UNLOADED) {
      data.should_show_discard_status = true;
    }
  }

  if (base::FeatureList::IsEnabled(tabs::kBraveRenamingTabs)) {
    tabs::TabInterface* const tab = model->GetTabAtIndex(index);
    CHECK(tab);

    TabUIHelper* const tab_ui_helper = TabUIHelper::From(tab);
    CHECK(tab_ui_helper);
    data.is_custom_title = tab_ui_helper->has_custom_title();
  }

  return data;
}
