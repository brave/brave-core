/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser_actions.h"

#include "base/check.h"
#include "base/check_deref.h"
#include "base/feature_list.h"
#include "base/functional/callback_helpers.h"
#include "base/types/to_address.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_news/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/brave_wayback_machine/buildflags/buildflags.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "brave/components/playlist/core/common/buildflags/buildflags.h"
#include "brave/components/psst/buildflags/buildflags.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/actions/chrome_action_id.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/side_panel/side_panel_action_callback.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_id.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_key.h"
#include "components/grit/brave_components_strings.h"
#include "ui/actions/actions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/vector_icon_types.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/browser/utils.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_NEWS)
#include "brave/components/brave_news/common/features.h"
#endif

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/components/containers/core/common/features.h"
#endif

#if BUILDFLAG(ENABLE_PLAYLIST)
#include "brave/components/playlist/core/browser/utils.h"
#endif

#if BUILDFLAG(ENABLE_PSST)
#include "brave/components/psst/core/common/features.h"
#include "chrome/browser/ui/page_action/page_action_triggers.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/browser/ui/tabs/public/brave_tab_features.h"
#include "brave/browser/ui/views/page_action/speedreader_page_action_controller.h"
#include "brave/components/speedreader/common/features.h"
#include "components/tabs/public/tab_interface.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
#include "brave/browser/ui/tabs/public/brave_tab_features.h"
#include "brave/browser/ui/views/page_action/wayback_machine_page_action_controller.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "components/tabs/public/tab_interface.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#endif

namespace {

#if BUILDFLAG(ENABLE_PLAYLIST) || BUILDFLAG(ENABLE_AI_CHAT) || \
    BUILDFLAG(ENABLE_BRAVE_NEWS) || BUILDFLAG(ENABLE_BRAVE_WALLET)
actions::ActionItem::ActionItemBuilder SidePanelAction(
    SidePanelEntryId id,
    int title_id,
    int tooltip_id,
    const gfx::VectorIcon& icon,
    actions::ActionId action_id,
    BrowserWindowInterface* bwi,
    bool is_pinnable) {
  return actions::ActionItem::Builder(
             CreateToggleSidePanelActionCallback(SidePanelEntryKey(id), bwi))
      .SetActionId(action_id)
      .SetText(l10n_util::GetStringUTF16(title_id))
      .SetTooltipText(l10n_util::GetStringUTF16(tooltip_id))
      .SetImage(ui::ImageModel::FromVectorIcon(icon, ui::kColorIcon))
      .SetProperty(actions::kActionItemPinnableKey, is_pinnable);
}
#endif  // BUILDFLAG(ENABLE_PLAYLIST) || BUILDFLAG(ENABLE_AI_CHAT) ||
        // BUILDFLAG(ENABLE_BRAVE_NEWS) || BUILDFLAG(ENABLE_BRAVE_WALLET)

}  // namespace

BraveBrowserActions::BraveBrowserActions(BrowserWindowInterface* bwi)
    : BrowserActions(bwi) {}

BraveBrowserActions::~BraveBrowserActions() = default;

void BraveBrowserActions::InitializeBrowserActions() {
  BrowserActions::InitializeBrowserActions();

  BrowserWindowInterface* const bwi = base::to_address(bwi_);

#if BUILDFLAG(ENABLE_PLAYLIST)
  if (playlist::IsPlaylistAllowed(profile_->GetPrefs())) {
    root_action_item_->AddChild(
        SidePanelAction(
            SidePanelEntryId::kPlaylist, IDS_SIDEBAR_PLAYLIST_ITEM_TITLE,
            IDS_SIDEBAR_PLAYLIST_ITEM_TITLE, kLeoProductPlaylistIcon,
            kActionSidePanelShowPlaylist, bwi, true)
            .Build());
  }
#endif  // BUILDFLAG(ENABLE_PLAYLIST)

#if BUILDFLAG(ENABLE_AI_CHAT)
  if (ai_chat::IsAIChatEnabled(profile_->GetPrefs())) {
    root_action_item_->AddChild(
        SidePanelAction(SidePanelEntryId::kChatUI, IDS_CHAT_UI_TITLE,
                        IDS_CHAT_UI_TITLE, kLeoProductBraveLeoIcon,
                        kActionSidePanelShowChatUI, bwi, true)
            .Build());
  }
#endif

#if BUILDFLAG(ENABLE_BRAVE_NEWS)
  if (base::FeatureList::IsEnabled(brave_news::features::kBraveNewsSidebar)) {
    root_action_item_->AddChild(
        SidePanelAction(SidePanelEntryId::kBraveNews, IDS_BRAVE_NEWS_TITLE,
                        IDS_BRAVE_NEWS_TITLE, kLeoRssIcon,
                        kActionSidePanelShowBraveNews, bwi, true)
            .Build());
  }
#endif

#if BUILDFLAG(ENABLE_CONTAINERS)
  if (base::FeatureList::IsEnabled(containers::features::kContainers)) {
    root_action_item_->AddChild(
        actions::ActionItem::Builder(
            // Safe to bind bwi to the callback because root_action_item_ is
            // going to be destroyed on the base class's destructor while the
            // browser window interface is also member of the base class.
            base::BindRepeating(
                [](BrowserWindowInterface* bwi, actions::ActionItem* item,
                   actions::ActionInvocationContext context) {
                  brave::OpenContainerMenuOnPageActionView(bwi, item);
                },
                bwi))
            .SetActionId(kActionShowPartitionedStorage)
            .SetEnabled(true)
            .Build());
  }
#endif

#if BUILDFLAG(ENABLE_PSST)
  if (base::FeatureList::IsEnabled(psst::features::kEnablePsst)) {
    root_action_item_->AddChild(
        actions::ActionItem::Builder(
            base::BindRepeating(
                [](BrowserWindowInterface* bwi, actions::ActionItem* item,
                   actions::ActionInvocationContext context) {
                  brave::OpenPsstMenuOnPageActionView(
                      bwi, item,
                      context.GetProperty(
                          page_actions::kBravePageActionEventFlagKey));
                },
                bwi))
            .SetActionId(kActionShowPsstIcon)
            .SetEnabled(true)
            .Build());
  }
#endif  // BUILDFLAG(ENABLE_PSST)

#if BUILDFLAG(ENABLE_SPEEDREADER)
  if (base::FeatureList::IsEnabled(
          speedreader::features::kSpeedreaderFeature)) {
    root_action_item_->AddChild(
        actions::ActionItem::Builder(
            base::BindRepeating(
                [](BrowserWindowInterface* bwi, actions::ActionItem* item,
                   actions::ActionInvocationContext context) {
                  tabs::BraveTabFeatures* const brave_tab_features =
                      tabs::BraveTabFeatures::FromTabFeatures(
                          bwi->GetActiveTabInterface()->GetTabFeatures());
                  CHECK(brave_tab_features);

                  page_actions::SpeedreaderPageActionController* const
                      controller = brave_tab_features
                                       ->speedreader_page_action_controller();
                  CHECK(controller);
                  controller->ExecuteAction(context.GetProperty(
                      page_actions::kBravePageActionEventFlagKey));
                },
                bwi))
            .SetActionId(kActionShowSpeedreader)
            .SetText(l10n_util::GetStringUTF16(
                IDS_SPEEDREADER_ICON_TURN_ON_READER_MODE))
            .SetTooltipText(l10n_util::GetStringUTF16(
                IDS_SPEEDREADER_ICON_TURN_ON_READER_MODE))
            .SetImage(ui::ImageModel::FromVectorIcon(kLeoProductSpeedreaderIcon,
                                                     ui::kColorIcon))
            .SetEnabled(true)
            .Build());
  }
#endif  // BUILDFLAG(ENABLE_SPEEDREADER)

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
  root_action_item_->AddChild(
      actions::ActionItem::Builder(
          base::BindRepeating(
              [](BrowserWindowInterface* bwi, actions::ActionItem* item,
                 actions::ActionInvocationContext context) {
                BrowserView& browser_view =
                    CHECK_DEREF(BrowserView::GetBrowserViewForBrowser(bwi));

                tabs::BraveTabFeatures* const brave_tab_features =
                    tabs::BraveTabFeatures::FromTabFeatures(
                        bwi->GetActiveTabInterface()->GetTabFeatures());
                CHECK(brave_tab_features);

                page_actions::WaybackMachinePageActionController* const
                    controller = brave_tab_features
                                     ->wayback_machine_page_action_controller();
                CHECK(controller);
                controller->ExecuteAction(
                    browser_view.toolbar_button_provider(), item);
              },
              bwi))
          .SetActionId(kActionShowWaybackMachine)
          .SetText(
              l10n_util::GetStringUTF16(IDS_BRAVE_WAYBACK_MACHINE_ICON_TOOLTIP))
          .SetTooltipText(
              l10n_util::GetStringUTF16(IDS_BRAVE_WAYBACK_MACHINE_ICON_TOOLTIP))
          .SetImage(
              ui::ImageModel::FromVectorIcon(kLeoHistoryIcon, ui::kColorIcon))
          .SetEnabled(true)
          .Build());
#endif  // BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
  if (brave_wallet::IsAllowed(profile_->GetPrefs()) &&
      base::FeatureList::IsEnabled(
          brave_wallet::features::kBraveWalletSidePanel)) {
    root_action_item_->AddChild(
        SidePanelAction(
            SidePanelEntryId::kWallet, IDS_SIDEBAR_WALLET_ITEM_TITLE,
            IDS_SIDEBAR_WALLET_ITEM_TITLE, kLeoProductBraveWalletIcon,
            kActionSidePanelShowWallet, bwi, true)
            .Build());
  }
#endif  // BUILDFLAG(ENABLE_BRAVE_WALLET)
}
