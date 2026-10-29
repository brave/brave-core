/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/peek/peek_contents.h"

#include <optional>
#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/tab_contents/tab_util.h"
#include "chrome/browser/ui/browser_web_contents_delegate/browser_web_contents_delegate.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tab_helpers.h"
#include "components/input/native_web_keyboard_event.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"
#include "content/public/browser/download_item_utils.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/common/input/web_input_event.h"
#include "third_party/blink/public/mojom/frame/fullscreen.mojom.h"
#include "ui/base/window_open_disposition.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "base/feature_list.h"
#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "brave/components/containers/core/common/features.h"
#include "content/public/browser/security_principal.h"
#include "content/public/browser/storage_partition_config.h"
#endif

namespace {

web_modal::WebContentsModalDialogManagerDelegate* GetModalDialogDelegate(
    content::WebContents* web_contents) {
  auto* manager =
      web_modal::WebContentsModalDialogManager::FromWebContents(web_contents);
  return manager ? manager->delegate() : nullptr;
}

#if BUILDFLAG(ENABLE_CONTAINERS)
// Peek must load in the source tab's container, like a link opened in a new
// tab would.
std::optional<content::StoragePartitionConfig> GetStoragePartitionToInherit(
    content::WebContents& source) {
  if (!base::FeatureList::IsEnabled(containers::features::kContainers)) {
    return std::nullopt;
  }
  return containers::MaybeInheritStoragePartition(
      source.GetPrimaryMainFrame()
          ->GetSiteInstance()
          ->GetSecurityPrincipal()
          .GetStoragePartitionConfig());
}
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

}  // namespace

PeekContents::PeekContents(BrowserWindowInterface& browser,
                           Delegate& delegate,
                           std::unique_ptr<content::WebContents> contents)
    : web_contents_(std::move(contents)),
      browser_(browser),
      browser_delegate_(
          CHECK_DEREF(BrowserWebContentsDelegate::From(&browser))),
      delegate_(delegate) {
  CHECK(web_contents_);
  TabHelpers::AttachTabHelpers(web_contents_.get());
  web_contents_->SetDelegate(this);

  // Without a modal dialog delegate, tab-modal dialogs such as HTTP auth
  // have nowhere to show.
  CHECK_DEREF(web_modal::WebContentsModalDialogManager::FromWebContents(
                  web_contents_.get()))
      .SetDelegate(this);

  // Prompts can't be shown while the "tab" is inactive. Trying to fails with
  // `OnPromptCreationFailedHiddenTab()`, at which point Peek moves into a tab,
  // where the prompt is attached to the tab and its omnibox.
  if (auto* manager = permissions::PermissionRequestManager::FromWebContents(
          web_contents_.get())) {
    manager->OnTabActiveStateChanged(false);
    permission_request_manager_observation_.Observe(manager);
  }

  if (auto* download_manager =
          web_contents_->GetBrowserContext()->GetDownloadManager()) {
    download_manager_observation_.Observe(download_manager);
  }
}

PeekContents::~PeekContents() {
  StopObserving();
  if (web_contents_) {
    web_contents_->SetDelegate(nullptr);
  }
}

// static
std::unique_ptr<content::WebContents> PeekContents::CreateWebContents(
    Profile* profile,
    const GURL& url,
    content::WebContents& source) {
  content::WebContents::CreateParams params(
      profile,
      tab_util::GetSiteInstanceForNewTab(profile, url
#if BUILDFLAG(ENABLE_CONTAINERS)
                                         ,
                                         GetStoragePartitionToInherit(source)
#endif
                                             ));
  return content::WebContents::Create(params);
}

// static
void PeekContents::OnMovedToTab(content::WebContents& contents) {
  if (auto* manager =
          permissions::PermissionRequestManager::FromWebContents(&contents)) {
    manager->OnTabActiveStateChanged(true);
  }
}

std::unique_ptr<content::WebContents> PeekContents::ReleaseWebContents() {
  StopObserving();
  if (web_contents_) {
    web_contents_->SetDelegate(nullptr);
  }
  return std::move(web_contents_);
}

void PeekContents::StopObserving() {
  permission_request_manager_observation_.Reset();
  download_manager_observation_.Reset();
  if (web_contents_) {
    if (auto* manager =
            web_modal::WebContentsModalDialogManager::FromWebContents(
                web_contents_.get());
        manager && manager->delegate() == this) {
      manager->SetDelegate(nullptr);
    }
  }
}

content::WebContents* PeekContents::OpenURLFromTab(
    content::WebContents* source,
    const content::OpenURLParams& params,
    base::OnceCallback<void(content::NavigationHandle&)>
        navigation_handle_callback) {
  if (params.disposition != WindowOpenDisposition::CURRENT_TAB) {
    return browser_->OpenURL(params, std::move(navigation_handle_callback));
  }

  base::WeakPtr<content::NavigationHandle> handle =
      source->GetController().LoadURLWithParams(
          content::NavigationController::LoadURLParams(params));
  if (handle && navigation_handle_callback) {
    std::move(navigation_handle_callback).Run(*handle);
  }
  return source;
}

content::WebContents* PeekContents::AddNewContents(
    content::WebContents* source,
    std::unique_ptr<content::WebContents> new_contents,
    const GURL& target_url,
    WindowOpenDisposition disposition,
    const blink::mojom::WindowFeatures& window_features,
    bool user_gesture,
    bool* was_blocked) {
  if (!user_gesture) {
    if (was_blocked) {
      *was_blocked = true;
    }
    return nullptr;
  }

  // The new window can script its opener, so the opener can't stay in Peek.
  delegate_->OnPeekMoveToTabRequired();
  // `this` is gone; `source` is now a tab with the browser as its delegate.
  content::WebContentsDelegate* tab_delegate = source->GetDelegate();
  if (!tab_delegate) {
    return nullptr;
  }
  return tab_delegate->AddNewContents(source, std::move(new_contents),
                                      target_url, disposition, window_features,
                                      user_gesture, was_blocked);
}

void PeekContents::CloseContents(content::WebContents* source) {
  delegate_->OnPeekCloseRequested();
}

void PeekContents::NavigationStateChanged(
    content::WebContents* source,
    content::InvalidateTypes changed_flags) {
  delegate_->OnPeekNavigationStateChanged();
}

void PeekContents::VisibleSecurityStateChanged(content::WebContents* source) {
  delegate_->OnPeekNavigationStateChanged();
}

bool PeekContents::TakeFocus(content::WebContents* source, bool reverse) {
  return delegate_->OnPeekTakeFocus(reverse);
}

bool PeekContents::HandleKeyboardEvent(
    content::WebContents* source,
    const input::NativeWebKeyboardEvent& event) {
  if (event.GetType() == blink::WebInputEvent::Type::kRawKeyDown &&
      event.windows_key_code == ui::VKEY_ESCAPE &&
      !(event.GetModifiers() & blink::WebInputEvent::kKeyModifiers)) {
    delegate_->OnPeekCloseRequested();
    return true;
  }
  return browser_delegate_->HandleKeyboardEvent(source, event);
}

void PeekContents::RunFileChooser(
    content::RenderFrameHost* render_frame_host,
    scoped_refptr<content::FileSelectListener> listener,
    const blink::mojom::FileChooserParams& params) {
  browser_delegate_->RunFileChooser(render_frame_host, std::move(listener),
                                    params);
}

void PeekContents::RequestMediaAccessPermission(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    content::MediaResponseCallback callback) {
  delegate_->OnPeekMoveToTabRequired();
  // `this` is gone; the tab's delegate shows the prompt.
  if (content::WebContentsDelegate* tab_delegate =
          web_contents->GetDelegate()) {
    tab_delegate->RequestMediaAccessPermission(web_contents, request,
                                               std::move(callback));
  }
}

bool PeekContents::CheckMediaAccessPermission(
    content::RenderFrameHost* render_frame_host,
    const url::Origin& security_origin,
    blink::mojom::MediaStreamType type) {
  return false;
}

void PeekContents::EnterFullscreenModeForTab(
    content::RenderFrameHost* requesting_frame,
    const blink::mojom::FullscreenOptions& options) {
  content::WebContents* contents =
      content::WebContents::FromRenderFrameHost(requesting_frame);
  delegate_->OnPeekMoveToTabRequired();
  // `this` is gone.
  if (content::WebContentsDelegate* tab_delegate = contents->GetDelegate()) {
    tab_delegate->EnterFullscreenModeForTab(requesting_frame, options);
  }
}

void PeekContents::RequestPointerLock(content::WebContents* web_contents,
                                      bool user_gesture,
                                      bool last_unlocked_by_target) {
  delegate_->OnPeekMoveToTabRequired();
  // `this` is gone.
  if (content::WebContentsDelegate* tab_delegate =
          web_contents->GetDelegate()) {
    tab_delegate->RequestPointerLock(web_contents, user_gesture,
                                     last_unlocked_by_target);
  }
}

void PeekContents::RequestKeyboardLock(content::WebContents* web_contents,
                                       bool esc_key_locked) {
  delegate_->OnPeekMoveToTabRequired();
  // `this` is gone.
  if (content::WebContentsDelegate* tab_delegate =
          web_contents->GetDelegate()) {
    tab_delegate->RequestKeyboardLock(web_contents, esc_key_locked);
  }
}

void PeekContents::SetWebContentsBlocked(content::WebContents* web_contents,
                                         bool blocked) {
  if (!blocked) {
    return;
  }
  delegate_->OnPeekMoveToTabRequired();
  // `this` is gone.
  if (auto* tab_delegate = GetModalDialogDelegate(web_contents)) {
    tab_delegate->SetWebContentsBlocked(web_contents, blocked);
  }
}

web_modal::WebContentsModalDialogHost*
PeekContents::GetWebContentsModalDialogHost(
    content::WebContents* web_contents) {
  delegate_->OnPeekMoveToTabRequired();
  // `this` is gone; the dialog is shown over the tab instead.
  auto* tab_delegate = GetModalDialogDelegate(web_contents);
  return tab_delegate
             ? tab_delegate->GetWebContentsModalDialogHost(web_contents)
             : nullptr;
}

void PeekContents::OnPromptCreationFailedHiddenTab() {
  delegate_->OnPeekMoveToTabOrCloseRequested();
}

void PeekContents::OnDownloadCreated(content::DownloadManager* manager,
                                     download::DownloadItem* item) {
  if (content::DownloadItemUtils::GetWebContents(item) == web_contents_.get()) {
    delegate_->OnPeekMoveToTabOrCloseRequested();
  }
}

void PeekContents::ManagerGoingDown(content::DownloadManager* manager) {
  download_manager_observation_.Reset();
}
