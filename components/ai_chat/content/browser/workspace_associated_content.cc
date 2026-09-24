// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/workspace_associated_content.h"

#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/logging.h"
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
#include "net/base/url_util.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/mojom/file_system_access/file_system_access_directory_handle.mojom.h"
#include "third_party/blink/public/mojom/web_launch/web_launch.mojom.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

// Builds the workspace:// URL used for set_url() on the delegate.
// Format: workspace://<uuid>/?folder=/path/to/folder
GURL BuildWorkspaceUrl(const std::string& uuid,
                       const base::FilePath& folder_path) {
  GURL url("workspace://" + uuid + "/");
  url = net::AppendQueryParameter(url, "folder", folder_path.AsUTF8Unsafe());
  CHECK(url.is_valid());
  return url;
}

// Parses a workspace:// URL and extracts the uuid and folder path.
// Returns true if parsing succeeded.
bool ParseWorkspaceUrl(const GURL& url,
                       std::string* uuid,
                       base::FilePath* folder_path) {
  if (!url.is_valid() || url.scheme() != "workspace") {
    return false;
  }
  *uuid = url.host();
  if (uuid->empty()) {
    return false;
  }
  std::string folder;
  if (!net::GetValueForKeyInQuery(url, "folder", &folder)) {
    return false;
  }
  *folder_path = base::FilePath::FromUTF8Unsafe(folder);
  return true;
}

// Builds the chrome-untrusted:// URL used for the actual WebContents load.
// Format: chrome-untrusted://<uuid>.leo-workspace/
GURL BuildChromeUntrustedUrl(const std::string& uuid) {
  return GURL(absl::StrFormat("%s://%s.%s/", content::kChromeUIUntrustedScheme,
                              uuid, kAIChatLeoWorkspaceUIHost));
}

}  // namespace

WorkspaceAssociatedContent::WorkspaceAssociatedContent(
    base::FilePath folder_path,
    content::BrowserContext* browser_context,
    base::OnceCallback<void(content::WebContents*)> attach_tab_helpers)
    : folder_path_(std::move(folder_path)) {
  std::string uuid = base::Uuid::GenerateRandomV4().AsLowercaseString();

  // Build the workspace:// URL for the delegate (used for display/sync).
  GURL workspace_url = BuildWorkspaceUrl(uuid, folder_path_);
  DVLOG(2) << __func__ << " creating workspace content at "
           << workspace_url.spec() << " for folder " << folder_path_;

  set_uuid(uuid);
  set_url(workspace_url);

  // The actual WebContents uses a chrome-untrusted:// URL where the uuid is
  // the page's subdomain, so that each workspace is a distinct origin and
  // doesn't share storage or File System Access grants with any other
  // workspace.
  GURL chrome_untrusted_url = BuildChromeUntrustedUrl(uuid);
  AttachWebContents(chrome_untrusted_url, browser_context,
                    std::move(attach_tab_helpers));
}

WorkspaceAssociatedContent::WorkspaceAssociatedContent(
    GURL url,
    content::BrowserContext* browser_context,
    base::OnceCallback<void(content::WebContents*)> attach_tab_helpers) {
  // Parse the workspace:// URL to extract the uuid and folder path.
  // Format: workspace://<uuid>/?folder=/path/to/folder
  std::string uuid;
  CHECK(ParseWorkspaceUrl(url, &uuid, &folder_path_))
      << "Invalid workspace URL: " << url.spec();

  DVLOG(2) << __func__ << " creating workspace content at " << url.spec()
           << " for folder " << folder_path_;

  set_uuid(uuid);
  set_url(url);

  // The actual WebContents uses a chrome-untrusted:// URL.
  GURL chrome_untrusted_url = BuildChromeUntrustedUrl(uuid);
  AttachWebContents(chrome_untrusted_url, browser_context,
                    std::move(attach_tab_helpers));
}

WorkspaceAssociatedContent::~WorkspaceAssociatedContent() = default;

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

void WorkspaceAssociatedContent::DocumentOnLoadCompletedInPrimaryMainFrame() {
  DVLOG(2) << __func__ << " workspace page loaded: " << url().spec();
  content::RenderFrameHost* rfh = web_contents_->GetPrimaryMainFrame();
  if (rfh && rfh->IsRenderFrameLive()) {
    DeliverDirectoryHandle(rfh);
  }
  // The page is now live and its handle delivered; mark it ready and attach so
  // the next generation loop harvests the tools it registers via WebMCP.
  page_ready_ = true;
  set_tools_attached(true);
}

void WorkspaceAssociatedContent::AttachWebContents(
    const GURL& url,
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
  content::NavigationController::LoadURLParams load_params(url);
  load_params.transition_type = ui::PAGE_TRANSITION_AUTO_TOPLEVEL;
  web_contents_->GetController().LoadURLWithParams(load_params);
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
          content::PathInfo(folder_path_),
          content::FileSystemAccessEntryFactory::UserAction::kOpen);
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

}  // namespace ai_chat
