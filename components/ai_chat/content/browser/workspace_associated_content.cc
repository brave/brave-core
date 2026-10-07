// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/workspace_associated_content.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/strcat.h"
#include "base/time/time.h"
#include "base/uuid.h"
#include "brave/components/ai_chat/content/browser/content_tool.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/permissions/permissions_client.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/file_system_access_entry_factory.h"
#include "content/public/browser/file_system_access_permission_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/mojom/file_system_access/file_system_access_directory_handle.mojom.h"
#include "third_party/blink/public/mojom/web_launch/web_launch.mojom.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"

namespace ai_chat {

namespace {

// The URL the workspace is identified by, and stored as: workspace://<uuid>.
GURL BuildWorkspaceUrl(std::string_view uuid) {
  return GURL(base::StrCat(
      {kAIChatWorkspaceScheme, url::kStandardSchemeSeparator, uuid}));
}

// The URL of the page that hosts the workspace's tools. Each workspace is
// served from its own subdomain, rather than a path under the workspace host,
// so that it is a distinct origin and shares neither storage nor File System
// Access grants with any other workspace.
// Format: chrome-untrusted://<uuid>.leo-workspace/
GURL BuildPageUrl(std::string_view uuid) {
  return GURL(base::StrCat({content::kChromeUIUntrustedScheme,
                            url::kStandardSchemeSeparator, uuid,
                            kAIChatLeoWorkspaceUIHostSuffix, "/"}));
}

// Extracts the uuid from a workspace://<uuid> URL, or returns std::nullopt if
// |url| is not one. The uuid must be a valid one, as it becomes a subdomain of
// the page's origin.
std::optional<std::string> ParseWorkspaceUrl(const GURL& url) {
  if (!url.is_valid() || !url.SchemeIs(kAIChatWorkspaceScheme) ||
      url.has_port() || url.has_username() || url.has_password() ||
      !url.path().empty() || url.has_query() || url.has_ref()) {
    return std::nullopt;
  }
  const base::Uuid uuid = base::Uuid::ParseLowercase(url.host());
  if (!uuid.is_valid()) {
    return std::nullopt;
  }
  return uuid.AsLowercaseString();
}

}  // namespace

WorkspaceAssociatedContent::WorkspaceAssociatedContent(
    std::optional<base::FilePath> folder_path,
    content::BrowserContext* browser_context,
    base::OnceCallback<void(content::WebContents*)> attach_tab_helpers)
    : WorkspaceAssociatedContent(
          base::Uuid::GenerateRandomV4().AsLowercaseString()) {
  folder_path_ = std::move(folder_path);
  DVLOG(2) << __func__ << " creating workspace " << url().spec()
           << " for folder " << folder_path_.value_or(base::FilePath());
  AttachWebContents(browser_context, std::move(attach_tab_helpers));
}

WorkspaceAssociatedContent::~WorkspaceAssociatedContent() = default;

// static
std::unique_ptr<WorkspaceAssociatedContent>
WorkspaceAssociatedContent::CreateFromUrl(
    GURL url,
    content::BrowserContext* browser_context,
    base::OnceCallback<void(content::WebContents*)> attach_tab_helpers) {
  // The FileSystemDirectoryHandle is restored from IndexedDB by the page.
  std::optional<std::string> uuid = ParseWorkspaceUrl(url);
  if (!uuid) {
    DVLOG(1) << "Invalid workspace URL: " << url.spec();
    return nullptr;
  }

  DVLOG(2) << __func__ << " restoring workspace " << url.spec();

  // WrapUnique, as the constructor is private.
  auto content =
      base::WrapUnique(new WorkspaceAssociatedContent(std::move(*uuid)));
  content->AttachWebContents(browser_context, std::move(attach_tab_helpers));
  return content;
}

void WorkspaceAssociatedContent::GetContent(GetPageContentCallback callback) {
  // Headless tool host: there is no page text to contribute to the
  // conversation. The value of this content is its tools, not its content.
  std::move(callback).Run(PageContent("", mojom::ContentType::Workspace));
}

void WorkspaceAssociatedContent::GetContentTools(
    GetContentToolsCallback callback) {
  // Report no tools until the page has loaded and registered them. This keeps
  // the manager's add-time probe synchronous (and empty), so the attach we do
  // in DocumentOnLoadCompletedInPrimaryMainFrame isn't clobbered by a late
  // probe callback for the initial (about:blank) document.
  content::RenderFrameHost* rfh = web_contents_->GetPrimaryMainFrame();
  if (!page_ready_ || !rfh || !rfh->IsRenderFrameLive()) {
    std::move(callback).Run({});
    return;
  }

  // |AIPageContentAgent| is bound to the RenderFrameHost, so keep the remote
  // alive only for this request by moving it into the callback.
  mojo::Remote<blink::mojom::AIPageContentAgent> agent;
  rfh->GetRemoteInterfaces()->GetInterface(agent.BindNewPipeAndPassReceiver());
  auto* agent_ptr = agent.get();
  auto options = blink::mojom::AIPageContentOptions::New();
  options->mode = blink::mojom::AIPageContentMode::kDefault;
  options->on_critical_path = true;
  agent_ptr->GetAIPageContent(
      std::move(options),
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(
          base::BindOnce(&WorkspaceAssociatedContent::OnContentToolsFetched,
                         weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                         rfh->GetWeakDocumentPtr(), std::move(agent)),
          nullptr));
}

void WorkspaceAssociatedContent::RunWhenPageReady(base::OnceClosure callback) {
  if (page_ready_) {
    std::move(callback).Run();
    return;
  }
  page_ready_callbacks_.push_back(std::move(callback));
}

void WorkspaceAssociatedContent::OnAssociatedWithConversation() {
  SubscribeToContentToolChanges();
}

url::Origin WorkspaceAssociatedContent::GetOrigin() const {
  // |url()| is only an identifier, so its origin is opaque. Tools run in the
  // page, so they belong to its origin.
  return url::Origin::Create(page_url_);
}

WorkspaceAssociatedContent::WorkspaceAssociatedContent(std::string uuid)
    : page_url_(BuildPageUrl(uuid)) {
  CHECK(page_url_.is_valid());
  set_url(BuildWorkspaceUrl(uuid));
  CHECK(url().is_valid());
  set_uuid(std::move(uuid));
}

void WorkspaceAssociatedContent::AttachWebContents(
    content::BrowserContext* browser_context,
    base::OnceCallback<void(content::WebContents*)> attach_tab_helpers) {
  CHECK(!web_contents())
      << "Should only attach once per WorkspaceAssociatedContent";
  SetTitle(u"Workspace");
  set_cached_page_content(PageContent("", mojom::ContentType::Workspace));

  // Hidden, headless background WebContents that hosts the workspace page.
  content::WebContents::CreateParams params(browser_context);
  params.initially_hidden = true;
  web_contents_ = content::WebContents::Create(params);
  std::move(attach_tab_helpers).Run(web_contents_.get());
  content::WebContentsObserver::Observe(web_contents_.get());

  // Load eagerly so the page can receive its handle and register tools before
  // the user sends a message.
  content::NavigationController::LoadURLParams load_params(page_url_);
  load_params.transition_type = ui::PAGE_TRANSITION_AUTO_TOPLEVEL;
  web_contents_->GetController().LoadURLWithParams(load_params);
}

void WorkspaceAssociatedContent::DocumentOnLoadCompletedInPrimaryMainFrame() {
  DVLOG(2) << __func__ << " workspace page loaded: " << url().spec();

  // Only deliver the directory handle for new workspaces. Restored workspaces
  // get their FileSystemDirectoryHandle from IndexedDB.
  if (folder_path_.has_value()) {
    content::RenderFrameHost* rfh = web_contents_->GetPrimaryMainFrame();
    if (rfh && rfh->IsRenderFrameLive()) {
      DeliverDirectoryHandle(rfh);
    }
  }

  // The page is now live and its handle delivered (or will be restored from
  // IndexedDB); mark it ready so GetContentTools can harvest tools.
  page_ready_ = true;

  // Now that the page is loaded, subscribe to tool changes. This may have been
  // called earlier in OnAssociatedWithConversation but the RFH wasn't ready.
  SubscribeToContentToolChanges();

  set_tools_attached(true);

  // Moved out first, as a callback may destroy |this|.
  for (auto& callback : std::exchange(page_ready_callbacks_, {})) {
    std::move(callback).Run();
  }
}

void WorkspaceAssociatedContent::OnContentToolsChanged() {
  NotifyContentToolsChanged();
}

void WorkspaceAssociatedContent::SubscribeToContentToolChanges() {
  if (!base::FeatureList::IsEnabled(blink::features::kWebMCP)) {
    return;
  }

  content::RenderFrameHost* rfh = web_contents_->GetPrimaryMainFrame();
  if (!rfh || !rfh->IsRenderFrameLive()) {
    return;
  }

  content_tools_extractor_.reset();
  rfh->GetRemoteInterfaces()->GetInterface(
      content_tools_extractor_.BindNewPipeAndPassReceiver());

  content_tools_listener_.reset();
  content_tools_extractor_->SetContentToolsListener(
      content_tools_listener_.BindNewPipeAndPassRemote());
}

void WorkspaceAssociatedContent::DeliverDirectoryHandle(
    content::RenderFrameHost* rfh) {
  // The permission grant below is an origin-scoped security decision, so it
  // uses the origin. |committed_url| is only used where the full URL is what's
  // wanted: the binding context's SafeBrowsing/Quarantine url, and the launch
  // url.
  const GURL origin_url = rfh->GetLastCommittedOrigin().GetURL();
  const GURL committed_url = rfh->GetLastCommittedURL();

  // Auto-grant File System Access read/write for the workspace origin so the
  // page can use the delivered handle without a permission prompt. The origin
  // is our own system-owned chrome-untrusted://<uuid>.leo-workspace page, and
  // access is scoped by the handle to the user-picked folder. Because the uuid
  // is part of the origin, this grants nothing to any other workspace.
  auto* map = permissions::PermissionsClient::Get()->GetSettingsMap(
      web_contents_->GetBrowserContext());
  map->SetContentSettingDefaultScope(
      origin_url, origin_url, ContentSettingsType::FILE_SYSTEM_READ_GUARD,
      CONTENT_SETTING_ALLOW);
  map->SetContentSettingDefaultScope(
      origin_url, origin_url, ContentSettingsType::FILE_SYSTEM_WRITE_GUARD,
      CONTENT_SETTING_ALLOW);

  // Mint a directory handle for the picked folder, bound to the workspace
  // frame.
  auto* factory = rfh->GetProcess()
                      ->GetStoragePartition()
                      ->GetFileSystemAccessEntryFactory();
  if (!factory) {
    return;
  }
  blink::mojom::FileSystemAccessEntryPtr entry =
      factory->CreateDirectoryEntryFromPath(
          content::FileSystemAccessEntryFactory::BindingContext(
              rfh->GetStorageKey(), committed_url, rfh->GetGlobalId()),
          content::PathInfo(*folder_path_),
          content::FileSystemAccessEntryFactory::AccessTrigger::kOpen);
  if (!entry) {
    return;
  }

  // Deliver the handle to the page's JS via window.launchQueue. Driving
  // WebLaunchService directly bypasses the WebApp/System-App gating that would
  // otherwise be required for a directory handle.
  std::vector<blink::mojom::FileSystemAccessEntryPtr> entries;
  entries.push_back(std::move(entry));
  mojo::AssociatedRemote<blink::mojom::WebLaunchService> launch_service;
  rfh->GetRemoteAssociatedInterfaces()->GetInterface(&launch_service);
  launch_service->EnqueueLaunchParams(committed_url, base::TimeTicks(),
                                      /*navigation_started=*/false,
                                      std::move(entries));
}

void WorkspaceAssociatedContent::OnContentToolsFetched(
    GetContentToolsCallback callback,
    content::WeakDocumentPtr rfh,
    mojo::Remote<blink::mojom::AIPageContentAgent> agent,
    blink::mojom::AIPageContentPtr result) {
  std::vector<std::unique_ptr<Tool>> tools;
  if (result && result->frame_data) {
    for (const auto& script_tool : result->frame_data->script_tools) {
      tools.push_back(std::make_unique<ContentTool>(*script_tool, rfh));
    }
  }
  std::move(callback).Run(std::move(tools));
}

}  // namespace ai_chat
