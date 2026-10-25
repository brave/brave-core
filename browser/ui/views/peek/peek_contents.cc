/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/peek/peek_contents.h"

#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_web_contents_delegate/browser_web_contents_delegate.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tab_helpers.h"
#include "components/input/native_web_keyboard_event.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/common/input/web_input_event.h"
#include "ui/base/window_open_disposition.h"
#include "ui/events/keycodes/keyboard_codes.h"

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
}

PeekContents::~PeekContents() {
  if (web_contents_) {
    web_contents_->SetDelegate(nullptr);
  }
}

// static
std::unique_ptr<content::WebContents> PeekContents::CreateWebContents(
    Profile* profile) {
  content::WebContents::CreateParams params(profile);
  return content::WebContents::Create(params);
}

std::unique_ptr<content::WebContents> PeekContents::ReleaseWebContents() {
  if (web_contents_) {
    web_contents_->SetDelegate(nullptr);
  }
  return std::move(web_contents_);
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
  return browser_delegate_->AddNewContents(
      source, std::move(new_contents), target_url, disposition, window_features,
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
  browser_delegate_->RequestMediaAccessPermission(web_contents, request,
                                                  std::move(callback));
}

bool PeekContents::CheckMediaAccessPermission(
    content::RenderFrameHost* render_frame_host,
    const url::Origin& security_origin,
    blink::mojom::MediaStreamType type) {
  return browser_delegate_->CheckMediaAccessPermission(render_frame_host,
                                                       security_origin, type);
}
