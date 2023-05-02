/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/ai_chat/ai_chat_side_panel_coordinator.h"

#include "brave/browser/ui/webui/ai_chat/ai_chat_ui.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/side_panel/side_panel_registry.h"
#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"

namespace {
class AIChatSidePanelWebView : public SidePanelWebUIView {
 public:
  METADATA_HEADER(AIChatSidePanelWebView);

  AIChatSidePanelWebView(Browser* browser,
                         base::RepeatingClosure close_cb,
                         BubbleContentsWrapper* contents_wrapper)
      : SidePanelWebUIView(
            /* on_show_cb = */ base::RepeatingClosure(),
            close_cb,
            contents_wrapper) {}
  AIChatSidePanelWebView(const AIChatSidePanelWebView&) = delete;
  AIChatSidePanelWebView& operator=(const AIChatSidePanelWebView&) = delete;
  ~AIChatSidePanelWebView() override = default;
};

BEGIN_METADATA(AIChatSidePanelWebView, views::WebView)
END_METADATA
}  // namespace

AIChatSidePanelCoordinator::AIChatSidePanelCoordinator(Browser* browser)
    : BrowserUserData<AIChatSidePanelCoordinator>(*browser) {}

AIChatSidePanelCoordinator::~AIChatSidePanelCoordinator() = default;

void AIChatSidePanelCoordinator::CreateAndRegisterEntry(
    SidePanelRegistry* global_registry) {
  global_registry->Register(std::make_unique<SidePanelEntry>(
      SidePanelEntry::Id::kChatUI,
      l10n_util::GetStringUTF16(IDS_SIDEBAR_CHAT_SUMMARIZER_ITEM_TITLE),
      ui::ImageModel(),
      base::BindRepeating(&AIChatSidePanelCoordinator::CreateWebView,
                          base::Unretained(this))));
}

std::unique_ptr<views::View> AIChatSidePanelCoordinator::CreateWebView() {
  const bool should_create_contents_wrapper = !contents_wrapper_;
  if (should_create_contents_wrapper) {
    contents_wrapper_ = std::make_unique<BubbleContentsWrapperT<AIChatUI>>(
        GURL(kChatUIURL), GetBrowser().profile(),
        IDS_SIDEBAR_CHAT_SUMMARIZER_ITEM_TITLE,
        /*webui_resizes_host=*/false,
        /*esc_closes_ui=*/false);
    contents_wrapper_->ReloadWebContents();
  }

  auto web_view = std::make_unique<AIChatSidePanelWebView>(
      &GetBrowser(), base::DoNothing(), contents_wrapper_.get());
  if (!should_create_contents_wrapper) {
    // SidePanelWebView's initial visibility is hidden. Thus, we need to
    // call this manually when we don't reload the web contents.
    // Calling this will also mark that the web contents is ready to go.
    web_view->ShowUI();
  }

  view_observation_.Observe(web_view.get());

  return web_view;
}

void AIChatSidePanelCoordinator::OnViewIsDeleting(views::View* view) {
  DestroyWebContentsIfNeeded();

  view_observation_.Reset();
}

void AIChatSidePanelCoordinator::DestroyWebContentsIfNeeded() {
  DCHECK(contents_wrapper_);
  contents_wrapper_.reset();
}

BROWSER_USER_DATA_KEY_IMPL(AIChatSidePanelCoordinator);
