// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/tabs/public/brave_tab_features.h"

#include <memory>
#include <utility>

#include "base/check.h"
#include "base/memory/ptr_util.h"
#include "base/no_destructor.h"
#include "brave/browser/ui/side_panel/brave_side_panel_utils.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tabs/public/tab_features.h"
#include "chrome/browser/ui/thumbnails/thumbnail_tab_helper.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "components/tabs/public/tab_interface.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/browser/ai_chat/ai_chat_utils.h"
#include "brave/browser/ai_chat/tab_data_web_contents_observer.h"
#endif

#if BUILDFLAG(ENABLE_PSST)
#include "brave/browser/psst/psst_ui_delegate_impl.h"
#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"
#endif

namespace tabs {

// static
BraveTabFeatures* BraveTabFeatures::FromTabFeatures(TabFeatures* tab_features) {
  return static_cast<BraveTabFeatures*>(tab_features);
}

BraveTabFeatures::BraveTabFeatures() = default;
BraveTabFeatures::~BraveTabFeatures() = default;

void BraveTabFeatures::Init(TabInterface& tab, Profile* profile) {
  TabFeatures::Init(tab, profile);

  // Expect upstream's Init to create the registry.
  CHECK(side_panel_registry());
  brave::RegisterContextualSidePanel(side_panel_registry(), tab.GetContents());

#if BUILDFLAG(ENABLE_AI_CHAT)
  if (ai_chat::IsAllowedForContext(profile)) {
    tab_data_observer_ = std::make_unique<ai_chat::TabDataWebContentsObserver>(
        tab.GetHandle().raw_value(), tab.GetContents());
  }
#endif

#if BUILDFLAG(ENABLE_PSST)
  psst_web_contents_observer_ =
      psst::PsstTabWebContentsObserver::MaybeCreateForWebContents(
          tab.GetContents(), profile,
          std::make_unique<psst::PsstUiDelegateImpl>(), profile->GetPrefs(),
          ISOLATED_WORLD_ID_BRAVE_INTERNAL);
#endif

  // Chromium's creation of this helper is gated on
  // features::kTabHoverCardImages, which we intentionally disable because we
  // want it to work differently. But we want the helper to be always available
  // so that we can switch hover modes via settings instead of having to restart
  // the browser.
  if (!thumbnail_tab_helper_) {
    thumbnail_tab_helper_ =
        GetUserDataFactory().CreateInstance<ThumbnailTabHelper>(tab, tab);
  }
}

}  // namespace tabs
