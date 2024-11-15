/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser_actions.h"

#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/playlist/common/buildflags/buildflags.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/side_panel/side_panel_action_callback.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry_id.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry_key.h"
#include "components/grit/brave_components_strings.h"
#include "ui/actions/actions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/vector_icon_types.h"

#if BUILDFLAG(ENABLE_PLAYLIST)
#include "brave/components/playlist/common/features.h"
#endif  // BUILDFLAG(ENABLE_PLAYLIST)

namespace {

actions::ActionItem::ActionItemBuilder SidePanelAction(
    SidePanelEntryId id,
    int title_id,
    int tooltip_id,
    const gfx::VectorIcon& icon,
    actions::ActionId action_id,
    Browser* browser,
    bool is_pinnable) {
  return actions::ActionItem::Builder(CreateToggleSidePanelActionCallback(
                                          SidePanelEntryKey(id), browser))
      .SetActionId(action_id)
      .SetText(l10n_util::GetStringUTF16(title_id))
      .SetTooltipText(l10n_util::GetStringUTF16(tooltip_id))
      .SetImage(ui::ImageModel::FromVectorIcon(icon, ui::kColorIcon))
      .SetProperty(actions::kActionItemPinnableKey, is_pinnable);
}

}  // namespace

BraveBrowserActions::BraveBrowserActions(Browser& browser)
    : BrowserActions(browser) {}

BraveBrowserActions::~BraveBrowserActions() = default;

void BraveBrowserActions::InitializeBrowserActions() {
  BrowserActions::InitializeBrowserActions();
  Browser* browser = &(browser_.get());

#if BUILDFLAG(ENABLE_PLAYLIST)
  if (base::FeatureList::IsEnabled(playlist::features::kPlaylist)) {
    root_action_item_->AddChild(
        SidePanelAction(
            SidePanelEntryId::kPlaylist, IDS_SIDEBAR_PLAYLIST_ITEM_TITLE,
            IDS_SIDEBAR_PLAYLIST_ITEM_TITLE, kLeoProductPlaylistIcon,
            kActionSidePanelShowPlaylist, browser, true)
            .Build());
  }
#endif

  Profile* profile = browser_->profile();
  if (ai_chat::IsAIChatEnabled(profile->GetPrefs())) {
    root_action_item_->AddChild(
        SidePanelAction(SidePanelEntryId::kChatUI, IDS_CHAT_UI_TITLE,
                        IDS_CHAT_UI_TITLE, kLeoProductBraveLeoIcon,
                        kActionSidePanelShowChatUI, browser, false)
            .Build());
  }
}
