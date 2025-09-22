// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/side_panel/ai_chat/ai_chat_side_panel_web_view.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/browser/ui/webui/ai_chat/ai_chat_ui.h"
#include "brave/components/ai_chat/core/common/ai_chat_urls.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

using SidePanelWebUIViewT_AIChatUI = SidePanelWebUIViewT<AIChatUI>;
BEGIN_TEMPLATE_METADATA(SidePanelWebUIViewT_AIChatUI, SidePanelWebUIViewT)
END_METADATA

// static
std::unique_ptr<views::View> AIChatSidePanelWebView::CreateView(
    Profile* profile,
    bool is_tab_associated,
    SidePanelEntryScope& scope) {
  CHECK(profile);

  auto web_view = std::make_unique<AIChatSidePanelWebView>(
      scope, std::make_unique<WebUIContentsWrapperT<AIChatUI>>(
                 is_tab_associated ? ai_chat::TabAssociatedConversationUrl()
                                   : GURL(kAIChatUIURL),
                 profile, IDS_SIDEBAR_CHAT_SUMMARIZER_ITEM_TITLE,
                 /*esc_closes_ui=*/false));
  web_view->ShowUI();
  return web_view;
}

AIChatSidePanelWebView::AIChatSidePanelWebView(
    SidePanelEntryScope& scope,
    std::unique_ptr<WebUIContentsWrapperT<AIChatUI>> contents_wrapper)
    : SidePanelWebUIViewT<AIChatUI>(
          scope,
          base::BindRepeating(&AIChatSidePanelWebView::OnShow,
                              base::Unretained(this)),
          base::RepeatingClosure(),
          std::move(contents_wrapper)) {}

AIChatSidePanelWebView::~AIChatSidePanelWebView() = default;

void AIChatSidePanelWebView::OnShow() {
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
