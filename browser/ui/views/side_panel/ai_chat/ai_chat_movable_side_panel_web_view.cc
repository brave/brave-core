// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/side_panel/ai_chat/ai_chat_movable_side_panel_web_view.h"

#include <memory>
#include <utility>

#include "base/check.h"
#include "base/functional/callback.h"
#include "brave/browser/ui/views/side_panel/ai_chat/ai_chat_side_panel_tab_transfer_bridge.h"
#include "brave/components/ai_chat/core/common/ai_chat_urls.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/file_select_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/navigator/browser_navigator.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_id.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_scope.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"
#include "chrome/browser/ui/webui/webui_embedding_context.h"
#include "components/tabs/public/tab_interface.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"
#include "content/public/browser/file_select_listener.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "extensions/buildflags/buildflags.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "ui/compositor/layer.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_EXTENSIONS_CORE)
#include "extensions/browser/view_type_utils.h"
#endif

namespace {

// Creates a fresh AI Chat `WebContents` for the side panel, wiring up the WebUI
// embedding context from `scope` (mirrors `SidePanelWebUIView`) so links, modal
// dialogs and the hosting browser resolve correctly.
std::unique_ptr<content::WebContents> CreateFreshAIChatContents(
    Profile* profile,
    bool is_tab_associated,
    SidePanelEntryScope& scope) {
  auto web_contents =
      content::WebContents::Create(content::WebContents::CreateParams(profile));

  if (scope.get_scope_type() == SidePanelEntryScope::ScopeType::kBrowser) {
    webui::SetBrowserWindowInterface(web_contents.get(),
                                     &scope.GetBrowserWindowInterface());
  } else {
    webui::SetTabInterface(web_contents.get(), &scope.GetTabInterface());
  }

  web_contents->GetController().LoadURLWithParams(
      content::NavigationController::LoadURLParams(
          is_tab_associated ? ai_chat::TabAssociatedConversationUrl()
                            : GURL(kAIChatUIURL)));
  return web_contents;
}

}  // namespace

// static
std::unique_ptr<views::View> AIChatMovableSidePanelWebView::CreateView(
    Profile* profile,
    bool is_tab_associated,
    SidePanelEntryScope& scope) {
  CHECK(profile);

  auto web_view = std::make_unique<AIChatMovableSidePanelWebView>(profile);

  // When a full-page AI Chat is being moved into the side panel, adopt that
  // live `WebContents` instead of creating a fresh one, so the conversation's
  // renderer/scroll/Mojo state is preserved.
  AIChatSidePanelTabTransferBridge* transfer_controller =
      scope.GetBrowserWindowInterface()
          .GetFeatures()
          .ai_chat_side_panel_tab_transfer_bridge();
  web_view->AdoptWebContents(
      transfer_controller && transfer_controller->HasPendingTransfer()
          ? transfer_controller->TakePendingContents()
          : CreateFreshAIChatContents(profile, is_tab_associated, scope));
  return web_view;
}

AIChatMovableSidePanelWebView::AIChatMovableSidePanelWebView(Profile* profile)
    : views::WebView(profile) {
  // Use the shared side panel web view id so the hosted contents is
  // discoverable by the same lookups the wrapper-based view supports (e.g.
  // `SidePanelCoordinator::GetWebContentsForTest` and the webui_browser side
  // panel), keeping parity with the flag-off `SidePanelWebUIView`.
  SetID(SidePanelWebUIView::kSidePanelWebViewId);
  // Paint to a non-opaque layer so the side panel's open/close content
  // transition can composite this view smoothly.
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
}

AIChatMovableSidePanelWebView::~AIChatMovableSidePanelWebView() {
  SetWebContents(nullptr);
}

void AIChatMovableSidePanelWebView::AdoptWebContents(
    std::unique_ptr<content::WebContents> web_contents) {
  owned_web_contents_ = std::move(web_contents);
  SetWebContents(owned_web_contents_.get());
}

void AIChatMovableSidePanelWebView::SetWebContents(
    content::WebContents* web_contents) {
  if (this->web_contents() == web_contents) {
    return;
  }

  if (this->web_contents() && !this->web_contents()->IsBeingDestroyed()) {
    this->web_contents()->WasHidden();
  }
  DetachWebContentsModalDialogManager(this->web_contents());

  AttachWebContentsModalDialogManager(web_contents);
  views::WebView::SetWebContents(web_contents);

  if (web_contents) {
    web_contents->WasShown();
    // Set `this` as the delegate to handle new tabs, links, file chooser and
    // media access requests.
    web_contents->SetDelegate(this);
#if BUILDFLAG(ENABLE_EXTENSIONS_CORE)
    // Mark as a component view. This keeps the side-panel contents out of the
    // extensions tab surface (e.g. chrome.tabs), and lets
    // `ChromeSpeechRecognitionManagerDelegate::CheckRenderFrameType()` permit
    // the Web Speech API that AI Chat uses for voice input.
    extensions::SetViewType(web_contents,
                            extensions::mojom::ViewType::kComponent);
#endif
  }
}

content::WebContents* AIChatMovableSidePanelWebView::AddNewContents(
    content::WebContents* source,
    std::unique_ptr<content::WebContents> new_contents,
    const GURL& target_url,
    WindowOpenDisposition disposition,
    const blink::mojom::WindowFeatures& window_features,
    bool user_gesture,
    bool* was_blocked) {
  BrowserWindowInterface* browser = webui::GetBrowserWindowInterface(source);
  if (!browser) {
    return nullptr;
  }

  // If AI Chat is not open in the side panel, don't open the tab.
  if (browser->GetFeatures().side_panel_ui()->GetCurrentEntryId() !=
      SidePanelEntryId::kChatUI) {
    return nullptr;
  }

  // Rather than opening a new tab from the side panel we navigate the active
  // tab next to the sidepanel.
  tabs::TabInterface* active_tab = browser->GetActiveTabInterface();
  NavigateParams params(browser, target_url, ui::PAGE_TRANSITION_LINK);

  // If the global side panel is enabled, open the url in the current active
  // tab. Otherwise open in a new tab.
  if (ai_chat::features::IsAIChatGlobalSidePanelEverywhereEnabled()) {
    params.source_contents = active_tab ? active_tab->GetContents() : nullptr;
    params.disposition = WindowOpenDisposition::CURRENT_TAB;
  } else {
    // We open in a new foreground tab so we don't start a new conversation.
    params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  }

  params.window_action = NavigateParams::WindowAction::kNoAction;
  params.user_gesture = user_gesture;

  Navigate(&params);

  return params.navigated_or_inserted_contents;
}

content::WebContents* AIChatMovableSidePanelWebView::OpenURLFromTab(
    content::WebContents* source,
    const content::OpenURLParams& params,
    base::OnceCallback<void(content::NavigationHandle&)>
        navigation_handle_callback) {
  BrowserWindowInterface* browser = webui::GetBrowserWindowInterface(source);
  if (browser &&
      (params.disposition == WindowOpenDisposition::NEW_FOREGROUND_TAB ||
       params.disposition == WindowOpenDisposition::NEW_BACKGROUND_TAB ||
       params.disposition == WindowOpenDisposition::NEW_WINDOW ||
       params.disposition == WindowOpenDisposition::OFF_THE_RECORD)) {
    return browser->OpenURL(params, std::move(navigation_handle_callback));
  }
  return nullptr;
}

void AIChatMovableSidePanelWebView::RunFileChooser(
    content::RenderFrameHost* render_frame_host,
    scoped_refptr<content::FileSelectListener> listener,
    const blink::mojom::FileChooserParams& params) {
  FileSelectHelper::RunFileChooser(render_frame_host, std::move(listener),
                                   params);
}

bool AIChatMovableSidePanelWebView::HandleKeyboardEvent(
    content::WebContents* source,
    const input::NativeWebKeyboardEvent& event) {
  return unhandled_keyboard_event_handler_.HandleKeyboardEvent(
      event, GetFocusManager());
}

web_modal::WebContentsModalDialogHost*
AIChatMovableSidePanelWebView::GetWebContentsModalDialogHost(
    content::WebContents* web_contents) {
  BrowserWindowInterface* browser =
      webui::GetBrowserWindowInterface(web_contents);
  if (!browser) {
    return nullptr;
  }
  return browser->GetWebContentsModalDialogHostForWindow();
}

void AIChatMovableSidePanelWebView::AttachWebContentsModalDialogManager(
    content::WebContents* web_contents) {
  if (!web_contents) {
    return;
  }
  web_modal::WebContentsModalDialogManager::CreateForWebContents(web_contents);
  web_modal::WebContentsModalDialogManager::FromWebContents(web_contents)
      ->SetDelegate(this);
}

void AIChatMovableSidePanelWebView::DetachWebContentsModalDialogManager(
    content::WebContents* web_contents) {
  if (!web_contents) {
    return;
  }
  auto* dialog_manager =
      web_modal::WebContentsModalDialogManager::FromWebContents(web_contents);
  if (dialog_manager) {
    dialog_manager->SetDelegate(nullptr);
  }
}
