/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shields/shields_settings_origin.h"

#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace brave_shields {

namespace {

// Returns the value of the page that initiated `handle`. Browser-initiated
// navigations have no such page, so they get an opaque origin (default
// settings).
url::Origin GetInitiatorValue(content::NavigationHandle* handle) {
  const auto& frame_token = handle->GetInitiatorFrameToken();
  if (!frame_token) {
    return url::Origin();
  }
  auto* initiator = content::RenderFrameHost::FromFrameToken(
      content::GlobalRenderFrameHostToken(handle->GetInitiatorProcessId(),
                                          *frame_token));
  // The initiator origin may belong to a subframe, so it says nothing about
  // the value of a page whose frame is gone.
  return initiator ? ShieldsSettingsOrigin::GetForFrame(initiator)
                   : url::Origin();
}

bool ShouldTrack(content::NavigationHandle* handle) {
  return handle->IsInOutermostMainFrame() && !handle->IsSameDocument() &&
         !handle->IsPageActivation();
}

void SetForDocument(content::RenderFrameHost* render_frame_host,
                    const url::Origin& value) {
  if (ShieldsSettingsOrigin::GetForCurrentDocument(render_frame_host)) {
    ShieldsSettingsOrigin::DeleteForCurrentDocument(render_frame_host);
  }
  ShieldsSettingsOrigin::CreateForCurrentDocument(render_frame_host, value);
}

}  // namespace

DOCUMENT_USER_DATA_KEY_IMPL(ShieldsSettingsOrigin);

ShieldsSettingsOrigin::ShieldsSettingsOrigin(
    content::RenderFrameHost* render_frame_host,
    const url::Origin& value)
    : DocumentUserData(render_frame_host), value_(value) {}

ShieldsSettingsOrigin::~ShieldsSettingsOrigin() = default;

// static
url::Origin ShieldsSettingsOrigin::Resolve(
    const std::optional<url::Origin>& origin,
    const GURL& url,
    const url::Origin& inherited) {
  if (origin && !origin->opaque()) {
    return *origin;
  }
  // Shields ignores sandboxing, so a document whose origin is opaque only
  // because it is sandboxed uses the origin of its URL.
  url::Origin url_origin = url::Origin::Create(url);
  if (!url_origin.opaque()) {
    return url_origin;
  }
  return inherited;
}

// static
url::Origin ShieldsSettingsOrigin::GetForFrame(
    content::RenderFrameHost* render_frame_host) {
  content::RenderFrameHost* main_frame =
      render_frame_host->GetOutermostMainFrame();
  if (const auto* data = GetForCurrentDocument(main_frame)) {
    return data->value();
  }
  return Resolve(main_frame->GetLastCommittedOrigin(),
                 main_frame->GetLastCommittedURL(), url::Origin());
}

// static
void ShieldsSettingsOrigin::SetForNewWindow(content::WebContents* new_contents,
                                            content::RenderFrameHost* opener) {
  // Only the initial empty document has no value of its own.
  if (!new_contents->GetController().IsInitialBlankNavigation()) {
    return;
  }
  SetForDocument(new_contents->GetPrimaryMainFrame(), GetForFrame(opener));
}

// static
void ShieldsSettingsOrigin::OnNavigationStarted(
    content::NavigationHandle* handle) {
  if (!ShouldTrack(handle)) {
    return;
  }
  const url::Origin inherited = GetInitiatorValue(handle);
  ShieldsSettingsOriginNavigationData::CreateForNavigationHandle(
      *handle, inherited, Resolve(std::nullopt, handle->GetURL(), inherited));
}

// static
void ShieldsSettingsOrigin::OnReadyToCommitNavigation(
    content::NavigationHandle* handle) {
  auto* data =
      ShieldsSettingsOriginNavigationData::GetForNavigationHandle(*handle);
  if (!data) {
    return;
  }
  data->set_value(Resolve(handle->GetOriginToCommit(), handle->GetURL(),
                          data->inherited()));
  // A new RenderFrameHost keeps document data set while it is pending commit,
  // so the new document has its value before it can make requests. A reused
  // RenderFrameHost still hosts the current document, which keeps its own
  // value until `OnNavigationFinished()`.
  content::RenderFrameHost* render_frame_host = handle->GetRenderFrameHost();
  if (render_frame_host &&
      render_frame_host->IsInLifecycleState(
          content::RenderFrameHost::LifecycleState::kPendingCommit)) {
    SetForDocument(render_frame_host, data->value());
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
  if (GetForCurrentDocument(render_frame_host)) {
    return;
  }
  auto* data =
      ShieldsSettingsOriginNavigationData::GetForNavigationHandle(*handle);
  CreateForCurrentDocument(render_frame_host,
                           data ? data->value()
                                : Resolve(handle->GetOriginToCommit(),
                                          handle->GetURL(), url::Origin()));
}

NAVIGATION_HANDLE_USER_DATA_KEY_IMPL(ShieldsSettingsOriginNavigationData);

ShieldsSettingsOriginNavigationData::ShieldsSettingsOriginNavigationData(
    content::NavigationHandle& handle,
    const url::Origin& inherited,
    const url::Origin& value)
    : inherited_(inherited), value_(value) {}

ShieldsSettingsOriginNavigationData::~ShieldsSettingsOriginNavigationData() =
    default;

}  // namespace brave_shields
