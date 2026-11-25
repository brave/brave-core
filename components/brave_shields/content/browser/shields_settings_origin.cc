/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/content/browser/shields_settings_origin.h"

#include <memory>
#include <string>

#include "base/notreached.h"
#include "base/supports_user_data.h"
#include "components/sessions/content/content_serialized_navigation_driver.h"
#include "components/sessions/content/extended_info_handler.h"
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
#include "services/network/public/cpp/resource_request.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace brave_shields {

namespace {

class DocumentSettingsOrigin
    : public content::DocumentUserData<DocumentSettingsOrigin> {
 public:
  const ShieldsSettingsOrigin value;

 private:
  // No public constructors to force going through static methods of
  // DocumentUserData (e.g. CreateForCurrentDocument).
  DocumentSettingsOrigin(content::RenderFrameHost* frame,
                         ShieldsSettingsOrigin origin)
      : DocumentUserData(frame), value(origin) {}

  friend content::DocumentUserData<DocumentSettingsOrigin>;
  DOCUMENT_USER_DATA_KEY_DECL();
};
DOCUMENT_USER_DATA_KEY_IMPL(DocumentSettingsOrigin);

struct InitiatorSettingsOrigin
    : content::NavigationHandleUserData<InitiatorSettingsOrigin> {
  InitiatorSettingsOrigin(content::NavigationHandle&,
                          ShieldsSettingsOrigin initiator)
      : inherited(initiator) {}
  const ShieldsSettingsOrigin inherited;
  NAVIGATION_HANDLE_USER_DATA_KEY_DECL();
};
NAVIGATION_HANDLE_USER_DATA_KEY_IMPL(InitiatorSettingsOrigin);

// Navigation entries can outlive the document and the page it inherited from.
struct NavigationEntrySettingsOrigin : base::SupportsUserData::Data {
  static constexpr char kKey = 0;

  explicit NavigationEntrySettingsOrigin(ShieldsSettingsOrigin origin)
      : value(origin) {}

  std::unique_ptr<Data> Clone() override {
    return std::make_unique<NavigationEntrySettingsOrigin>(value);
  }

  const ShieldsSettingsOrigin value;
};

const NavigationEntrySettingsOrigin* GetNavigationEntrySettingsOrigin(
    content::NavigationHandle* handle) {
  if (!handle->IsHistory() || handle->WasServerRedirect() ||
      handle->GetNetErrorCode() != net::OK) {
    return nullptr;
  }
  const auto* entry = handle->GetNavigationEntry();
  return entry ? static_cast<const NavigationEntrySettingsOrigin*>(
                     entry->GetUserData(&NavigationEntrySettingsOrigin::kKey))
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
  if (DocumentSettingsOrigin::GetForCurrentDocument(render_frame_host)) {
    DocumentSettingsOrigin::DeleteForCurrentDocument(render_frame_host);
  }
  DocumentSettingsOrigin::CreateForCurrentDocument(render_frame_host, value);
}

}  // namespace

// Only the origin is session data. An absent field selects the ordinary
// fallback (profile defaults for an inherited page without an initiator).
// The stored value is an unversioned origin URL, and saved sessions outlive
// browser updates. If the format changes, add a version marker and keep
// restoring this format.
class ShieldsSettingsOrigin::SessionHandler
    : public sessions::ExtendedInfoHandler {
 public:
  std::string GetExtendedInfo(content::NavigationEntry* entry) const override {
    const auto* data = static_cast<const NavigationEntrySettingsOrigin*>(
        entry->GetUserData(&NavigationEntrySettingsOrigin::kKey));
    return data ? data->value.GetURL().spec() : std::string();
  }

  void RestoreExtendedInfo(const std::string& info,
                           content::NavigationEntry* entry) override {
    const auto origin = url::Origin::Create(GURL(info));
    if (!origin.opaque() && origin.GetURL().spec() == info) {
      entry->SetUserData(&NavigationEntrySettingsOrigin::kKey,
                         std::make_unique<NavigationEntrySettingsOrigin>(
                             ShieldsSettingsOrigin(origin)));
    }
  }
};

// static
bool ShieldsSettingsOrigin::UsesFactoryContext(
    content::ContentBrowserClient::URLLoaderFactoryType type) {
  using Type = content::ContentBrowserClient::URLLoaderFactoryType;
  switch (type) {
    case Type::kWorkerMainResource:
    case Type::kWorkerSubResource:
    case Type::kServiceWorkerScript:
    case Type::kServiceWorkerSubResource:
      return true;
    case Type::kNavigation:
    case Type::kDownload:
    case Type::kDocumentSubResource:
    case Type::kPrefetch:
    case Type::kDevTools:
    case Type::kEarlyHints:
      return false;
  }
  NOTREACHED();
}

void ShieldsSettingsOrigin::RegisterSessionHandler() {
  sessions::ContentSerializedNavigationDriver::GetInstance()
      ->RegisterExtendedInfoHandler("brave_shields_settings_origin",
                                    std::make_unique<SessionHandler>());
}

ShieldsSettingsOrigin::ShieldsSettingsOrigin(const url::Origin& origin)
    : url_(origin.GetURL()) {}

// static
ShieldsSettingsOrigin ShieldsSettingsOrigin::ForFrame(
    content::RenderFrameHost& frame) {
  const auto* main = frame.GetOutermostMainFrame();
  if (const auto* data = DocumentSettingsOrigin::GetForCurrentDocument(main)) {
    return data->value;
  }
  const auto& origin = main->GetLastCommittedOrigin();
  if (!origin.opaque()) {
    return ShieldsSettingsOrigin(origin);
  }
  const GURL& url = main->GetLastCommittedURL();
  return url.SchemeIsHTTPOrHTTPS()
             ? ShieldsSettingsOrigin(url::Origin::Create(url))
             : ShieldsSettingsOrigin();
}

// static
std::optional<ShieldsSettingsOrigin>
ShieldsSettingsOrigin::ForEmbeddedDocumentFactory(
    const content::GlobalRenderFrameHostToken& token,
    content::ContentBrowserClient::URLLoaderFactoryType type) {
  if (type == content::ContentBrowserClient::URLLoaderFactoryType::
                  kDocumentSubResource) {
    auto* frame = content::RenderFrameHost::FromFrameToken(token);
    if (frame && frame != frame->GetOutermostMainFrame()) {
      return ForFrame(*frame);
    }
  }
  return std::nullopt;
}

// static
ShieldsSettingsOrigin ShieldsSettingsOrigin::ForRequest(
    const network::ResourceRequest& request,
    const content::GlobalRenderFrameHostToken& token,
    std::optional<content::ContentBrowserClient::URLLoaderFactoryType>
        factory_type,
    base::optional_ref<const net::IsolationInfo> factory_info,
    base::optional_ref<const ShieldsSettingsOrigin> document_key) {
  const net::IsolationInfo* info = nullptr;
  if (request.trusted_params) {
    info = &request.trusted_params->isolation_info;
  } else if (factory_type && UsesFactoryContext(*factory_type) &&
             factory_info) {
    info = &*factory_info;
  }
  const ShieldsSettingsOrigin top_frame_value =
      info ? ForIsolationInfo(*info) : ShieldsSettingsOrigin();
  // A main-frame request's token can still name the outgoing document. The
  // resource type is not used: requests a service worker refetches keep
  // kMainFrame.
  if (info &&
      info->request_type() == net::IsolationInfo::RequestType::kMainFrame) {
    return top_frame_value;
  }
  if (document_key) {
    return *document_key;
  }
  if (auto* frame = content::RenderFrameHost::FromFrameToken(token)) {
    return ForFrame(*frame);
  }
  return top_frame_value;
}

ShieldsSettingsOrigin ShieldsSettingsOrigin::ForRedirect(
    const network::ResourceRequest& request) const {
  if (request.trusted_params &&
      request.trusted_params->isolation_info.request_type() ==
          net::IsolationInfo::RequestType::kMainFrame) {
    return ForIsolationInfo(request.trusted_params->isolation_info);
  }
  return *this;
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
  if (const auto* saved = GetNavigationEntrySettingsOrigin(&handle)) {
    return saved->value;
  }
  // about:blank can inherit a subframe's DOM origin, but its Shields key
  // is the one captured from the initiating outermost document.
  if (handle.GetURL().IsAboutBlank() || handle.GetURL().IsAboutSrcdoc()) {
    const auto* data = InitiatorSettingsOrigin::GetForNavigationHandle(handle);
    return data ? data->inherited : ShieldsSettingsOrigin();
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
  const auto* data = InitiatorSettingsOrigin::GetForNavigationHandle(handle);
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
  InitiatorSettingsOrigin::CreateForNavigationHandle(*handle,
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
  if (!DocumentSettingsOrigin::GetForCurrentDocument(render_frame_host)) {
    DocumentSettingsOrigin::CreateForCurrentDocument(render_frame_host,
                                                     ForCommit(*handle));
  }
  auto& controller = render_frame_host->GetController();
  auto* entry = controller.GetLastCommittedEntry();
  const auto& key =
      DocumentSettingsOrigin::GetForCurrentDocument(render_frame_host)->value;
  const auto* prior = static_cast<const NavigationEntrySettingsOrigin*>(
      entry->GetUserData(&NavigationEntrySettingsOrigin::kKey));
  const bool session_data_changed =
      prior ? prior->value.GetURL() != key.GetURL() : !key.GetURL().is_empty();
  entry->SetUserData(&NavigationEntrySettingsOrigin::kKey,
                     std::make_unique<NavigationEntrySettingsOrigin>(key));
  // The session service already serialized the entry at
  // NavigationEntryCommitted.
  if (session_data_changed) {
    controller.NotifyEntryChanged(entry);
  }
}

}  // namespace brave_shields
