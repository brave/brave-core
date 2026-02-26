/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/tabs/tab_renderer_data.h"

#include "base/check.h"

#define FromTabInterface FromTabInterface_ChromiumImpl
#include <chrome/browser/ui/tabs/tab_renderer_data.cc>
#undef FromTabInterface

#include "brave/browser/ui/tabs/shared_pinned_tab_service.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service_factory.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/resource_coordinator/tab_load_tracker.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/common/webui_url_constants.h"
#include "components/grit/brave_components_scaled_resources.h"
#include "extensions/buildflags/buildflags.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/color_utils.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/extension_web_ui.h"
#endif

namespace {

// Returns a value indicating whether the specified URL is a chrome URL that has
// been overridden by an extension.
bool IsChromeURLOverridden(const GURL& url, Profile* profile) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  GURL override_url(url);
  return ExtensionWebUI::HandleChromeURLOverride(&override_url, profile);
#else
  return false;
#endif
}

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

TabRendererData TabRendererData::FromTabInterface(tabs::TabInterface* tab) {
  auto* const bwi = tab->GetBrowserWindowInterface();
  if (base::FeatureList::IsEnabled(tabs::kBraveSharedPinnedTabs)) {
    // Note that in unit tests, this may be null.
    if (bwi) {
      TabStripModel* model = bwi->GetTabStripModel();
      const int index = model->GetIndexOfTab(tab);
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
  }

  auto data = FromTabInterface_ChromiumImpl(tab);

  content::WebContents* const contents = tab->GetContents();
  const GURL& url = contents->GetVisibleURL();

  // Override favicon theming for some WebUIs.
  if (url.SchemeIs(content::kChromeUIScheme)) {
    if (url.host() == chrome::kChromeUINewTabHost) {
      if (bwi && !IsChromeURLOverridden(url, bwi->GetProfile())) {
        data.favicon = GetThemedNTPFavicon(contents);
        data.should_themify_favicon = false;
      }
    } else if (url.host() == kWelcomeHost || url.host() == kRewardsPageHost) {
      data.should_themify_favicon = false;
    }
  }

  // Show which tabs are unloaded.
  if (!data.should_show_discard_status) {
    const auto loading_state =
        resource_coordinator::TabLoadTracker::Get()->GetLoadingState(contents);
    if (loading_state ==
        resource_coordinator::TabLoadTracker::LoadingState::UNLOADED) {
      data.should_show_discard_status = true;
    }
  }
  return data;
}
