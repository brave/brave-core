// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/side_panel/ai_chat/ai_chat_side_panel_contents_wrapper.h"

#include <utility>

#include "brave/browser/ui/webui/ai_chat/ai_chat_ui.h"

AIChatSidePanelContentsWrapper::AIChatSidePanelContentsWrapper(
    const GURL& webui_url,
    Profile* profile,
    int task_manager_string_id,
    bool esc_closes_ui)
    : WebUIContentsWrapperT<AIChatUI>(webui_url,
                                      profile,
                                      task_manager_string_id,
                                      esc_closes_ui) {}

AIChatSidePanelContentsWrapper::~AIChatSidePanelContentsWrapper() = default;

void AIChatSidePanelContentsWrapper::SetTargetURLChangedCallback(
    base::RepeatingCallback<void(const GURL&)> callback) {
  target_url_changed_callback_ = std::move(callback);
}

void AIChatSidePanelContentsWrapper::UpdateTargetURL(
    content::WebContents* source,
    const GURL& url) {
  if (target_url_changed_callback_) {
    target_url_changed_callback_.Run(url);
  }
}
