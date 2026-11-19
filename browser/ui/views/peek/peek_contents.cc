/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/peek/peek_contents.h"

#include <optional>
#include <utility>

#include "base/check.h"
#include "base/feature_list.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "chrome/browser/favicon/favicon_utils.h"
#include "chrome/browser/history/history_tab_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/tab_contents/tab_util.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "components/input/native_web_keyboard_event.h"
#include "content/public/browser/keyboard_event_processing_result.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/security_principal.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/common/input/web_input_event.h"
#include "ui/base/window_open_disposition.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "brave/components/containers/core/common/features.h"
#endif

namespace {

// Tab features are only created for contents in a tab, but Peek needs these
// before then: the favicon is shown in the card and visits are recorded in
// history. Once the contents are in a tab, the tab features reuse them.
void AttachHelpersFromTabFeatures(content::WebContents& web_contents) {
  favicon::CreateContentFaviconDriverForWebContents(&web_contents);
  if (!Profile::FromBrowserContext(web_contents.GetBrowserContext())
           ->IsOffTheRecord()) {
    HistoryTabHelper::GetOrCreateForWebContents(&web_contents);
  }
}

const content::StoragePartitionConfig& GetStoragePartitionConfig(
    content::WebContents& web_contents) {
  return web_contents.GetSiteInstance()
      ->GetSecurityPrincipal()
      .GetStoragePartitionConfig();
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
      GetStoragePartitionConfig(source));
}
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

}  // namespace

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
  std::unique_ptr<content::WebContents> contents =
      content::WebContents::Create(params);
  // Fail closed rather than load the page with another partition's cookies
  // and storage, e.g. a container tab's link in the default partition.
  if (GetStoragePartitionConfig(*contents) !=
      GetStoragePartitionConfig(source)) {
    return nullptr;
  }
  return contents;
}

PeekContents::PeekContents(BrowserWindowInterface& browser,
                           Delegate& delegate,
                           std::unique_ptr<content::WebContents> contents)
    : web_contents_(std::move(contents)),
      browser_(browser),
      delegate_(delegate) {
  CHECK(web_contents_);
  AttachHelpersFromTabFeatures(*web_contents_);
  web_contents_->SetDelegate(this);
}

PeekContents::~PeekContents() {
  if (web_contents_) {
    web_contents_->SetDelegate(nullptr);
  }
}

content::WebContents* PeekContents::OpenURLFromTab(
    content::WebContents* source,
    const content::OpenURLParams& params,
    base::OnceCallback<void(content::NavigationHandle&)>
        navigation_handle_callback) {
  if (params.disposition != WindowOpenDisposition::kCurrentTab) {
    // A new URL should be opened in the browser rather than in Peek.
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

content::KeyboardEventProcessingResult PeekContents::PreHandleKeyboardEvent(
    content::WebContents* source,
    const input::NativeWebKeyboardEvent& event) {
  // Escape is handled before the page sees it, so the page can't keep the user
  // in Peek by consuming it. Unconsumed events also never come back from a
  // hidden renderer, e.g. an occluded window.
  if (event.GetType() == blink::WebInputEvent::Type::kRawKeyDown &&
      event.windows_key_code == ui::VKEY_ESCAPE &&
      !(event.GetModifiers() & blink::WebInputEvent::kKeyModifiers)) {
    delegate_->OnPeekCloseRequested();
    return content::KeyboardEventProcessingResult::HANDLED;
  }
  return content::KeyboardEventProcessingResult::NOT_HANDLED;
}
