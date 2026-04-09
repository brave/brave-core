/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/wallet_bubble_manager_delegate_impl.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/scoped_observation.h"
#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/wallet_webui_contents_wrapper.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_common_ui.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_panel_ui.h"
#include "build/build_config.h"
#include "chrome/browser/file_select_helper.h"
#include "chrome/browser/picture_in_picture/picture_in_picture_occlusion_observer.h"
#include "chrome/browser/picture_in_picture/scoped_picture_in_picture_occlusion_observation.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/top_container_view.h"
#include "chrome/browser/ui/webui/top_chrome/webui_contents_wrapper.h"
#include "components/grit/brave_components_strings.h"
#include "components/input/native_web_keyboard_event.h"
#include "content/public/browser/keyboard_event_processing_result.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/mojom/choosers/file_chooser.mojom-forward.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/dialog_client_view.h"
#include "url/gurl.h"

namespace brave_wallet {

// Wallet WebUI bubble with security-related input handling aligned with
// PermissionPromptBaseView: no default dialog button, PiP occlusion tracking,
// and filtering of unintended keyboard input before it reaches the WebContents.
class WalletWebUIBubbleDialogView : public WebUIBubbleDialogView,
                                    public PictureInPictureOcclusionObserver {
  METADATA_HEADER(WalletWebUIBubbleDialogView, WebUIBubbleDialogView)

 public:
  WalletWebUIBubbleDialogView(views::View* anchor_view,
                              WebUIContentsWrapper* contents_wrapper,
                              const std::optional<gfx::Rect>& anchor_rect,
                              views::BubbleBorder::Arrow arrow)
      : WebUIBubbleDialogView(anchor_view,
                              contents_wrapper->GetWeakPtr(),
                              anchor_rect,
                              arrow) {
    set_close_on_deactivate(false);
    SetDefaultButton(static_cast<int>(ui::mojom::DialogButton::kNone));

    CHECK(anchor_widget());
    anchor_widget_observation_.Observe(
        anchor_widget()->GetPrimaryWindowWidget());
  }

  WalletWebUIBubbleDialogView(const WalletWebUIBubbleDialogView&) = delete;
  WalletWebUIBubbleDialogView& operator=(const WalletWebUIBubbleDialogView&) =
      delete;
  ~WalletWebUIBubbleDialogView() override = default;

  bool ShouldBlockKeyboardEventForSecurity(
      const input::NativeWebKeyboardEvent& event) {
    if (occluded_by_picture_in_picture_) {
      return true;
    }

    views::DialogClientView* client_view = GetDialogClientView();
    if (!client_view) {
      return false;
    }

#if defined(USE_AURA)
    const ui::Event* native_event = event.os_event;
    if (!native_event || !native_event->IsKeyEvent()) {
      return false;
    }
    const ui::KeyEvent* key_event = native_event->AsKeyEvent();
    if (!key_event) {
      return false;
    }
    return client_view->IsPossiblyUnintendedInteraction(
        *key_event, /*allow_key_events=*/false);
#else
    if (!event.os_event) {
      return false;
    }
    ui::KeyEvent key_event(event.os_event);
    return client_view->IsPossiblyUnintendedInteraction(
        static_cast<const ui::Event&>(key_event), /*allow_key_events=*/false);
#endif
  }

  void AddedToWidget() override {
    WebUIBubbleDialogView::AddedToWidget();
    StartTrackingPictureInPictureOcclusion();
  }

  bool ShouldIgnoreButtonPressedEventHandling(
      views::View* button,
      const ui::Event& event) const override {
    return occluded_by_picture_in_picture_;
  }

  bool ShouldDescendIntoChildForEventHandling(
      gfx::NativeView child,
      const gfx::Point& location) override {
    if (occluded_by_picture_in_picture_) {
      return false;
    }
    return WebUIBubbleDialogView::ShouldDescendIntoChildForEventHandling(
        child, location);
  }

  content::KeyboardEventProcessingResult PreHandleKeyboardEvent(
      content::WebContents* source,
      const input::NativeWebKeyboardEvent& event) override {
    if (ShouldBlockKeyboardEventForSecurity(event)) {
      return content::KeyboardEventProcessingResult::HANDLED;
    }
    return WebUIBubbleDialogView::PreHandleKeyboardEvent(source, event);
  }

  void RunFileChooser(content::RenderFrameHost* render_frame_host,
                      scoped_refptr<content::FileSelectListener> listener,
                      const blink::mojom::FileChooserParams& params) override {
    FileSelectHelper::RunFileChooser(render_frame_host, std::move(listener),
                                     params);
  }

  void OnOcclusionStateChanged(bool occluded) override {
    if (occluded_by_picture_in_picture_ && !occluded) {
      TriggerInputProtection();
    }
    occluded_by_picture_in_picture_ = occluded;
  }

 private:
  void StartTrackingPictureInPictureOcclusion() {
    occlusion_observation_.Observe(GetWidget());
  }

  // WidgetObserver:
  void OnWidgetTreeActivated(views::Widget* root_widget,
                             views::Widget* active_widget) override {
    // Similar to what `ExtensionPopup::OnWidgetTreeActivated` does.

    if (!GetWidget()->IsVisible()) {
      return;
    }

    if (active_widget != GetWidget()) {
      GetWidget()->CloseWithReason(views::Widget::ClosedReason::kLostFocus);
    }
  }

  base::ScopedObservation<views::Widget, views::WidgetObserver>
      anchor_widget_observation_{this};

  ScopedPictureInPictureOcclusionObservation occlusion_observation_{this};
  bool occluded_by_picture_in_picture_ = false;
};

BEGIN_METADATA(WalletWebUIBubbleDialogView)
END_METADATA

class WalletWebUIBubbleManager : public WebUIBubbleManagerImpl<WalletPanelUI> {
 public:
  WalletWebUIBubbleManager(views::View* anchor_view,
                           Browser* browser,
                           const GURL& webui_url,
                           int task_manager_string_id,
                           bool force_load_on_create)
      : WebUIBubbleManagerImpl<WalletPanelUI>(anchor_view,
                                              browser,
                                              webui_url,
                                              task_manager_string_id,
                                              force_load_on_create),
        browser_(browser),
        anchor_view_(anchor_view),
        webui_url_(webui_url),
        task_manager_string_id_(task_manager_string_id) {}

  base::WeakPtr<WebUIBubbleDialogView> CreateWebUIBubbleDialog(
      const std::optional<gfx::Rect>& anchor,
      views::BubbleBorder::Arrow arrow) override {
    if (!cached_contents_wrapper()) {
      set_cached_contents_wrapper(std::make_unique<WalletWebUIContentsWrapper>(
          webui_url_, browser_->profile(), task_manager_string_id_));
    }

    // This is prevent duplicate logic of cached_contents_wrapper creation
    // so we close WebUIBubbleDialogView and re-create bubble with
    // WalletWebUIBubbleDialogView.
    auto bubble_view_to_close =
        WebUIBubbleManagerImpl<WalletPanelUI>::CreateWebUIBubbleDialog(anchor,
                                                                       arrow);
    auto* widget_to_close = bubble_view_to_close->GetWidget();
    if (widget_to_close) {
      widget_to_close->CloseNow();
    }
    auto* contents_wrapper = cached_contents_wrapper();
    CHECK(contents_wrapper);
    auto bubble_view = std::make_unique<WalletWebUIBubbleDialogView>(
        anchor_view_, contents_wrapper, anchor, arrow);
    bubble_view_ = bubble_view->GetWeakPtr();
    auto* widget =
        views::BubbleDialogDelegateView::CreateBubble(std::move(bubble_view));
    CHECK(widget);

    // Checking if we create WalletPanelUI instance of WebUI and
    // extracting WebUIContentsWrapper class to pass real browser delegate
    // into it to redirect popups to be opened as separate windows.
    // Set a callback to be possible to activate/deactivate wallet panel from
    // typescript side
    if (!contents_wrapper->web_contents()) {
      return bubble_view_;
    }
    content::WebUI* const webui = contents_wrapper->web_contents()->GetWebUI();
    if (!webui || !webui->GetController()) {
      return bubble_view_;
    }
    WalletPanelUI* wallet_panel =
        webui->GetController()->template GetAs<WalletPanelUI>();
    if (!wallet_panel || !browser_ || !browser_->AsWeakPtr()) {
      return bubble_view_;
    }
    // Set Browser delegate to redirect popups to be opened as Popup window
    contents_wrapper->SetWebContentsAddNewContentsDelegate(
        browser_->AsWeakPtr());

    return bubble_view_;
  }

  void CloseOpenedPopups() {
    auto* contents_wrapper = cached_contents_wrapper();
    if (!contents_wrapper) {
      return;
    }
    for (auto tab_id : contents_wrapper->popup_ids()) {
      Browser* popup_browser = nullptr;
      content::WebContents* popup_contents =
          brave_wallet::GetWebContentsFromTabId(&popup_browser, tab_id);
      if (!popup_contents || !popup_browser) {
        continue;
      }
      base::WeakPtr<content::WebContentsDelegate> delegate =
          popup_browser->AsWeakPtr();
      if (!delegate) {
        continue;
      }
      delegate->CloseContents(popup_contents);
    }
    contents_wrapper->ClearPopupIds();
  }

  const std::vector<int32_t>& GetPopupIdsForTesting() {
    auto* contents_wrapper = cached_contents_wrapper();
    return contents_wrapper->popup_ids();
  }

  void OnWidgetDestroying(views::Widget* widget) override {
    CloseOpenedPopups();
    WebUIBubbleManagerImpl<WalletPanelUI>::OnWidgetDestroying(widget);
  }

  content::WebContents* GetWebContentsForTesting() {
    if (!bubble_view_) {
      return nullptr;
    }
    return bubble_view_->web_view()->GetWebContents();
  }

 private:
  const raw_ptr<Browser> browser_;
  const raw_ptr<views::View> anchor_view_;
  const GURL webui_url_;
  const int task_manager_string_id_;
  base::WeakPtr<WebUIBubbleDialogView> bubble_view_ = nullptr;
  base::WeakPtrFactory<WalletWebUIBubbleManager> weak_factory_{this};
};

// static
std::unique_ptr<WalletBubbleManagerDelegate>
WalletBubbleManagerDelegate::MaybeCreate(content::WebContents* web_contents,
                                         const GURL& webui_url) {
  if (!IsAllowedForContext(web_contents->GetBrowserContext())) {
    return nullptr;
  }

  return std::make_unique<WalletBubbleManagerDelegateImpl>(web_contents,
                                                           webui_url);
}

WalletBubbleManagerDelegateImpl::WalletBubbleManagerDelegateImpl(
    content::WebContents* web_contents,
    const GURL& webui_url)
    : web_contents_(web_contents), webui_url_(webui_url) {
  Browser* browser = chrome::FindBrowserWithTab(web_contents_);
  DCHECK(browser);

  views::View* anchor_view;
  if (browser->is_type_normal()) {
    anchor_view = static_cast<BraveBrowserView*>(browser->window())
                      ->GetWalletButtonAnchorView();
  } else {
    anchor_view = static_cast<BrowserView*>(browser->window())->top_container();
  }

  DCHECK(anchor_view);
  webui_bubble_manager_ = std::make_unique<WalletWebUIBubbleManager>(
      anchor_view, browser, webui_url_, IDS_ACCNAME_BRAVE_WALLET_BUTTON,
      /*force_load_on_create=*/false);
}

WalletBubbleManagerDelegateImpl::~WalletBubbleManagerDelegateImpl() {
  webui_bubble_manager_->CloseBubble();
}

void WalletBubbleManagerDelegateImpl::ShowBubble() {
  // Suppress request if not from active web_contents.
  if (GetActiveWebContents() != web_contents_) {
    return;
  }

  webui_bubble_manager_->ShowBubble();
}

content::WebContents*
WalletBubbleManagerDelegateImpl::GetWebContentsForTesting() {
  return webui_bubble_manager_->GetWebContentsForTesting();
}

const std::vector<int32_t>&
WalletBubbleManagerDelegateImpl::GetPopupIdsForTesting() {
  return webui_bubble_manager_->GetPopupIdsForTesting();
}
void WalletBubbleManagerDelegateImpl::CloseBubble() {
  webui_bubble_manager_->CloseBubble();
}

bool WalletBubbleManagerDelegateImpl::IsShowingBubble() {
  return webui_bubble_manager_ && webui_bubble_manager_->GetBubbleWidget();
}

bool WalletBubbleManagerDelegateImpl::IsBubbleClosedForTesting() {
  return !webui_bubble_manager_ || !webui_bubble_manager_->GetBubbleWidget() ||
         webui_bubble_manager_->GetBubbleWidget()->IsClosed();
}

}  // namespace brave_wallet
