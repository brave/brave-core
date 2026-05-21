/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser_actions.h"

#include "base/functional/callback_helpers.h"
#include "base/types/to_address.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "brave/components/playlist/core/common/buildflags/buildflags.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/actions/chrome_action_id.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/side_panel/side_panel_action_callback.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_id.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_key.h"
#include "chrome/browser/ui/tab_search_feature.h"
#include "components/grit/brave_components_strings.h"
#include "ui/actions/actions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/vector_icon_types.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/browser/utils.h"
#endif

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/components/containers/core/common/features.h"
#endif

#if BUILDFLAG(ENABLE_PLAYLIST)
#include "brave/components/playlist/core/browser/utils.h"
#endif

namespace {

#if BUILDFLAG(ENABLE_PLAYLIST) || BUILDFLAG(ENABLE_AI_CHAT)
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
#endif  // BUILDFLAG(ENABLE_PLAYLIST) || BUILDFLAG(ENABLE_AI_CHAT)

}  // namespace

BraveBrowserActions::BraveBrowserActions(BrowserWindowInterface* bwi)
    : BrowserActions(bwi) {}

BraveBrowserActions::~BraveBrowserActions() = default;

void BraveBrowserActions::InitializeBrowserActions() {
  BrowserActions::InitializeBrowserActions();

  // browser_actions.cc initializes kActionTabSearch as kNotPinnable.
  // In Brave, HasTabSearchToolbarButton() is always true and the button is
  // shown via the pinned-actions model, so
  // ToolbarController::GetDefaultResponsiveElements() must see it as kPinnable
  // when the toolbar is initialized — otherwise kActionTabSearch is missing
  // from responsive_elements_ and the overflow menu crashes when clicked if
  // search buton is the only item in overflow menu.
  if (features::HasTabSearchToolbarButton()) {
    auto* tab_search_action = actions::ActionManager::Get().FindAction(
        kActionTabSearch, root_action_item_.get());
    if (tab_search_action) {
      tab_search_action->SetProperty(
          actions::kActionItemPinnableKey,
          static_cast<std::underlying_type_t<actions::ActionPinnableState>>(
              actions::ActionPinnableState::kPinnable));
    }
  }

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
}
