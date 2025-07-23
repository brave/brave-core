/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/browser/ai_chat/ai_chat_urls.h"
#include "brave/browser/ui/webui/ai_chat/ai_chat_ui.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/side_panel/side_panel_registry.h"
#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"
#include "components/grit/brave_components_strings.h"
#include "components/user_prefs/user_prefs.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/widget/widget.h"

using SidePanelWebUIViewT_AIChatUI = SidePanelWebUIViewT<AIChatUI>;
BEGIN_TEMPLATE_METADATA(SidePanelWebUIViewT_AIChatUI, SidePanelWebUIViewT)
END_METADATA

namespace {

// A custom web view to set focus correctly when the side panel is shown.
class AIChatSidePanelWebView : public SidePanelWebUIViewT<AIChatUI> {
 public:
  AIChatSidePanelWebView(
      SidePanelEntryScope& scope,
      std::unique_ptr<WebUIContentsWrapperT<AIChatUI>> contents_wrapper)
      : SidePanelWebUIViewT<AIChatUI>(
            scope,
            base::BindRepeating(&AIChatSidePanelWebView::OnShow,
                                base::Unretained(this)),
            base::RepeatingClosure(),
            std::move(contents_wrapper)) {}

  ~AIChatSidePanelWebView() override = default;

 private:
  // This callback is invoked multiple times, so we need to ensure that
  // focus is set only once with `should_focus_`.
  void OnShow() {
    if (!should_focus_) {
      return;
    }

    if (IsFocusable()) {
      auto* widget = GetWidget();
      CHECK(widget);
      // There's a bug in focus handling. We should clear focus before setting
      // side panel focused. Otherwise, focus won't be forwarded to the
      // web contents properly.
      widget->GetFocusManager()->ClearFocus();
      RequestFocus();
      should_focus_ = false;
    }
  }

  bool should_focus_ = true;
};

std::unique_ptr<views::View> CreateAIChatSidePanelWebView(
    base::WeakPtr<Profile> profile,
    SidePanelEntryScope& scope) {
  CHECK(profile);

  auto web_view = std::make_unique<AIChatSidePanelWebView>(
      scope, std::make_unique<WebUIContentsWrapperT<AIChatUI>>(
                 ai_chat::TabAssociatedConversationUrl(), profile.get(),
                 IDS_SIDEBAR_CHAT_SUMMARIZER_ITEM_TITLE,
                 /*esc_closes_ui=*/false));
  web_view->ShowUI();
  return web_view;
}

}  // namespace

namespace brave {

// Register here for an entry that is used for all tabs and its life time is
// tied with tab. If it has specific life time, use separated manager for
// registering it.
void RegisterContextualSidePanel(SidePanelRegistry* registry,
                                 content::WebContents* web_contents) {
  content::BrowserContext* context = web_contents->GetBrowserContext();
  if (ai_chat::IsAIChatEnabled(user_prefs::UserPrefs::Get(context)) &&
      Profile::FromBrowserContext(context)->IsRegularProfile()) {
    // If |registry| already has it, it's no-op.
    registry->Register(std::make_unique<SidePanelEntry>(
        SidePanelEntry::Key(SidePanelEntry::Id::kChatUI),
        base::BindRepeating(&CreateAIChatSidePanelWebView,
                            Profile::FromBrowserContext(context)->GetWeakPtr()),
        SidePanelEntry::kSidePanelDefaultContentWidth));
  }
}

}  // namespace brave
