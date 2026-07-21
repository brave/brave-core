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
#include "chrome/browser/ui/page_action/action_ids.h"
#include "chrome/browser/ui/page_action/page_action_controller.h"
#include "chrome/browser/ui/side_panel/side_panel_registry.h"
#include "chrome/browser/ui/tabs/public/tab_features.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "components/tabs/public/tab_interface.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/browser/containers/container_tab_tracker.h"
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
#include "brave/browser/psst/psst_ui_desktop_presenter.h"
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
  auto* side_panel_registry = SidePanelRegistry::From(&tab);
  CHECK(side_panel_registry);
  brave::RegisterContextualSidePanel(side_panel_registry, tab.GetContents());

#if BUILDFLAG(ENABLE_AI_CHAT)
  if (ai_chat::IsAllowedForContext(profile)) {
    tab_data_observer_ = std::make_unique<ai_chat::TabDataWebContentsObserver>(
        tab.GetHandle().raw_value(), tab.GetContents());
  }
#endif

#if BUILDFLAG(ENABLE_PSST)
  // The page action controller can be null when features::kPageActionsMigration
  // is disabled, and the PSST page action may be absent even when it exists
  // (e.g. in unit tests that don't register the browser's action items), so
  // guard on ActionExists() to avoid operating on an unregistered page action
  // model.
  if (base::FeatureList::IsEnabled(features::kPageActionsMigration) &&
      base::FeatureList::IsEnabled(psst::features::kEnablePsst) &&
      page_action_controller()->ActionExists(kActionShowPsstIcon)) {
    psst_action_controller_ =
        std::make_unique<page_actions::PsstActionController>(
            tab, *page_action_controller());
    psst_web_contents_observer_ =
        psst::PsstTabWebContentsObserver::MaybeCreateForWebContents(
            tab.GetContents(), profile,
            std::make_unique<psst::PsstUiDelegateImpl>(
                PsstSettingsServiceFactory::GetForProfile(profile),
                profile->GetPrefs(),
                std::make_unique<psst::PsstUiDesktopPresenter>(
                    tab.GetContents()->GetWeakPtr(),
                    psst_action_controller_->AsWeakPtr())),
            profile->GetPrefs(), ISOLATED_WORLD_ID_BRAVE_INTERNAL);
  }
#endif

#if BUILDFLAG(ENABLE_CONTAINERS)
  if (base::FeatureList::IsEnabled(containers::features::kContainers)) {
    container_tab_tracker_ = containers::ContainerTabTracker::MaybeCreate(tab);
    // In case of features::kPageActionsMigration is disabled, this controller
    // can be null. The feature is enabled by default. So note that we don't
    // show the partitioned storage page action when the features is disabled
    // by users. The page action itself may also be absent even when the
    // controller exists (e.g. in unit tests that don't register the browser's
    // action items), so guard on ActionExists() to avoid creating a controller
    // that would operate on an unregistered page action model.
    if (base::FeatureList::IsEnabled(features::kPageActionsMigration) &&
        page_action_controller()->ActionExists(kActionShowPartitionedStorage)) {
      partitioned_storage_page_action_controller_ = std::make_unique<
          page_actions::PartitionedStoragePageActionController>(
          tab, *page_action_controller());
      partitioned_storage_page_action_controller_->Init();
    }
  }
#endif
}

}  // namespace tabs
