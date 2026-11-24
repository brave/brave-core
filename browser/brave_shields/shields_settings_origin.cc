/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shields/shields_settings_origin.h"

#include <memory>

#include "base/supports_user_data.h"
#include "content/public/browser/document_user_data.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_handle_user_data.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "net/base/isolation_info.h"
#include "net/base/net_errors.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace brave_shields {

namespace {

struct DocumentOrigin : content::DocumentUserData<DocumentOrigin> {
  DocumentOrigin(content::RenderFrameHost* frame, ShieldsSettingsOrigin origin)
      : DocumentUserData(frame), value(origin) {}
  const ShieldsSettingsOrigin value;
  DOCUMENT_USER_DATA_KEY_DECL();
};
DOCUMENT_USER_DATA_KEY_IMPL(DocumentOrigin);

struct NavigationOrigin : content::NavigationHandleUserData<NavigationOrigin> {
  NavigationOrigin(content::NavigationHandle&, ShieldsSettingsOrigin initiator)
      : inherited(initiator) {}
  const ShieldsSettingsOrigin inherited;
  NAVIGATION_HANDLE_USER_DATA_KEY_DECL();
};
NAVIGATION_HANDLE_USER_DATA_KEY_IMPL(NavigationOrigin);

// History entries can outlive the document and the page it inherited from.
struct HistorySettingsOrigin : base::SupportsUserData::Data {
  static constexpr char kKey = 0;

  explicit HistorySettingsOrigin(ShieldsSettingsOrigin origin)
      : value(origin) {}

  std::unique_ptr<Data> Clone() override {
    return std::make_unique<HistorySettingsOrigin>(value);
  }

  const ShieldsSettingsOrigin value;
};

const HistorySettingsOrigin* GetHistorySettingsOrigin(
    content::NavigationHandle* handle) {
  if (!handle->IsHistory() || handle->WasServerRedirect() ||
      handle->GetNetErrorCode() != net::OK) {
    return nullptr;
  }
  const auto* entry = handle->GetNavigationEntry();
  return entry ? static_cast<const HistorySettingsOrigin*>(
                     entry->GetUserData(&HistorySettingsOrigin::kKey))
               : nullptr;
}

// Returns the value of the page that initiated `handle`. Browser-initiated
// navigations have no such page, so they get an opaque origin (default
// settings).
ShieldsSettingsOrigin GetInitiatorValue(content::NavigationHandle* handle) {
  const auto& frame_token = handle->GetInitiatorFrameToken();
  if (!frame_token) {
    return {};
  }
  auto* initiator = content::RenderFrameHost::FromFrameToken(
      content::GlobalRenderFrameHostToken(handle->GetInitiatorProcessId(),
                                          *frame_token));
  // The initiator origin may belong to a subframe, so it says nothing about
  // the value of a page whose frame is gone.
  return initiator ? ShieldsSettingsOrigin::ForFrame(*initiator)
                   : ShieldsSettingsOrigin();
}

bool ShouldTrack(content::NavigationHandle* handle) {
  return handle->IsInOutermostMainFrame() && !handle->IsSameDocument() &&
         !handle->IsPageActivation();
}

void SetForDocument(content::RenderFrameHost* render_frame_host,
                    ShieldsSettingsOrigin value) {
  if (DocumentOrigin::GetForCurrentDocument(render_frame_host)) {
    DocumentOrigin::DeleteForCurrentDocument(render_frame_host);
  }
  DocumentOrigin::CreateForCurrentDocument(render_frame_host, value);
}

}  // namespace

ShieldsSettingsOrigin::ShieldsSettingsOrigin(const url::Origin& origin)
    : url_(origin.GetURL()) {}

// static
ShieldsSettingsOrigin ShieldsSettingsOrigin::ForFrame(
    content::RenderFrameHost& frame) {
  const auto* main = frame.GetOutermostMainFrame();
  if (const auto* data = DocumentOrigin::GetForCurrentDocument(main)) {
    return data->value;
  }
  const auto& origin = main->GetLastCommittedOrigin();
  return ShieldsSettingsOrigin(
      origin.opaque() ? url::Origin::Create(main->GetLastCommittedURL())
                      : origin);
}

// static
ShieldsSettingsOrigin ShieldsSettingsOrigin::ForIsolationInfo(
    const net::IsolationInfo& info) {
  return info.top_frame_origin()
             ? ShieldsSettingsOrigin(*info.top_frame_origin())
             : ShieldsSettingsOrigin();
}

// static
ShieldsSettingsOrigin ShieldsSettingsOrigin::ForCommit(
    content::NavigationHandle& handle) {
  if (const auto* saved = GetHistorySettingsOrigin(&handle)) {
    return saved->value;
  }
  const auto origin_to_commit =
      handle.HasCommitted()
          ? handle.GetRenderFrameHost()->GetLastCommittedOrigin()
          : handle.GetOriginToCommit();
  if (origin_to_commit && !origin_to_commit->opaque()) {
    return ShieldsSettingsOrigin(*origin_to_commit);
  }
  // A sandboxed HTTP(S) document still uses its own site's settings.
  auto origin = url::Origin::Create(handle.GetURL());
  if (!origin.opaque()) {
    return ShieldsSettingsOrigin(origin);
  }
  const auto* data = NavigationOrigin::GetForNavigationHandle(handle);
  return data ? data->inherited : ShieldsSettingsOrigin();
}

// static
void ShieldsSettingsOrigin::SetForNewWindow(content::WebContents* new_contents,
                                            content::RenderFrameHost* opener) {
  // Only the initial empty document has no value of its own.
  if (!new_contents->GetController().IsInitialBlankNavigation()) {
    return;
  }
  SetForDocument(new_contents->GetPrimaryMainFrame(), ForFrame(*opener));
}

// static
void ShieldsSettingsOrigin::OnNavigationStarted(
    content::NavigationHandle* handle) {
  if (!ShouldTrack(handle)) {
    return;
  }
  NavigationOrigin::CreateForNavigationHandle(*handle,
                                              GetInitiatorValue(handle));
}

// static
void ShieldsSettingsOrigin::OnReadyToCommitNavigation(
    content::NavigationHandle* handle) {
  if (!ShouldTrack(handle)) {
    return;
  }
  // A new RenderFrameHost keeps document data set while it is pending commit,
  // so the new document has its value before it can make requests. A reused
  // RenderFrameHost still hosts the current document, which keeps its own
  // value until `OnNavigationFinished()`.
  content::RenderFrameHost* render_frame_host = handle->GetRenderFrameHost();
  if (render_frame_host &&
      render_frame_host->IsInLifecycleState(
          content::RenderFrameHost::LifecycleState::kPendingCommit)) {
    SetForDocument(render_frame_host, ForCommit(*handle));
  }
}

// static
void ShieldsSettingsOrigin::OnNavigationFinished(
    content::NavigationHandle* handle) {
  if (!ShouldTrack(handle) || !handle->HasCommitted()) {
    return;
  }
  content::RenderFrameHost* render_frame_host = handle->GetRenderFrameHost();
  // The document already has a value if it was set while pending commit, or
  // if the commit kept the initial empty document of a new window.
  if (!DocumentOrigin::GetForCurrentDocument(render_frame_host)) {
    DocumentOrigin::CreateForCurrentDocument(render_frame_host,
                                             ForCommit(*handle));
  }
  auto* entry = render_frame_host->GetController().GetLastCommittedEntry();
  entry->SetUserData(
      &HistorySettingsOrigin::kKey,
      std::make_unique<HistorySettingsOrigin>(
          DocumentOrigin::GetForCurrentDocument(render_frame_host)->value));
}

}  // namespace brave_shields
