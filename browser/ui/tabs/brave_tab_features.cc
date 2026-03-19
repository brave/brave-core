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
#include "brave/browser/psst/psst_ui_desktop_presenter.h"
#include "brave/browser/ui/side_panel/brave_side_panel_utils.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tabs/public/tab_features.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "components/tabs/public/tab_interface.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/browser/ui/views/page_action/partitioned_storage_page_action_controller.h"
#include "brave/components/containers/core/common/features.h"
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/browser/ai_chat/ai_chat_utils.h"
#include "brave/browser/ai_chat/tab_data_web_contents_observer.h"
#endif

#if BUILDFLAG(ENABLE_PSST)
#include "brave/browser/psst/psst_settings_service_factory.h"
#include "brave/browser/psst/psst_ui_delegate_impl.h"
#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"
#include "brave/components/psst/common/features.h"
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
  if (base::FeatureList::IsEnabled(psst::features::kEnablePsst)) {
    psst_web_contents_observer_ =
        psst::PsstTabWebContentsObserver::MaybeCreateForWebContents(
            tab.GetContents(), profile,
            std::make_unique<psst::PsstUiDelegateImpl>(
                PsstSettingsServiceFactory::GetForProfile(profile),
                profile->GetPrefs(),
                std::make_unique<psst::PsstUiDesktopPresenter>(
                    tab.GetContents())),
            profile->GetPrefs(), ISOLATED_WORLD_ID_BRAVE_INTERNAL);
  }
#endif

#if BUILDFLAG(ENABLE_CONTAINERS)
  if (page_action_controller() &&
      base::FeatureList::IsEnabled(containers::features::kContainers)) {
    // In case of features::kPageActionsMigration is disabled, this controller
    // can be null. The feature is enabled by default. So note that we don't
    // show the partitioned storage page action when the features is disabled
    // by users.
    partitioned_storage_page_action_controller_ =
        std::make_unique<page_actions::PartitionedStoragePageActionController>(
            tab, *page_action_controller());
    partitioned_storage_page_action_controller_->Init();
  }
#endif
}

}  // namespace tabs
