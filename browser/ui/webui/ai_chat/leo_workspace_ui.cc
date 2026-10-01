// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/leo_workspace_ui.h"

#include <string>
#include <string_view>

#include "base/check.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/leo_workspace_util.h"
#include "brave/components/ai_chat/resources/grit/ai_chat_ui_generated_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "components/grit/brave_components_resources.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/service_worker_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "net/base/schemeful_site.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "third_party/blink/public/common/service_worker/service_worker_status_code.h"
#include "third_party/blink/public/common/storage_key/storage_key.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_registration_options.mojom.h"
#include "third_party/blink/public/mojom/storage_key/ancestor_chain_bit.mojom-shared.h"
#include "ui/webui/webui_util.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"

namespace ai_chat {

namespace {

constexpr char kViewerServiceWorkerScript[] = "leo_workspace_view_sw.bundle.js";

std::string UntrustedOrigin(std::string_view host) {
  return base::StrCat(
      {content::kChromeUIUntrustedScheme, url::kStandardSchemeSeparator, host});
}

// Creates a data source with restrictive baseline CSPs. Callers should override
// frame-src, frame-ancestors, and worker-src as needed for their use case.
content::WebUIDataSource* CreateAndAddDataSource(
    content::BrowserContext* browser_context,
    std::string_view host,
    int default_resource_id) {
  content::WebUIDataSource* source = content::WebUIDataSource::CreateAndAdd(
      browser_context, UntrustedOrigin(host) + "/");

  webui::SetupWebUIDataSource(source, kAiChatUiGenerated, default_resource_id);

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::DefaultSrc, "default-src 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src 'self' chrome-untrusted://resources;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      "style-src 'self' chrome-untrusted://resources;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ConnectSrc, "connect-src 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ObjectSrc, "object-src 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc, "frame-src 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameAncestors,
      "frame-ancestors 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::WorkerSrc, "worker-src 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FormAction, "form-action 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::BaseURI, "base-uri 'none';");

  return source;
}

void CreateAndAddViewerDataSource(content::BrowserContext* browser_context,
                                  const GURL& viewer_url,
                                  std::string_view workspace_host) {
  content::WebUIDataSource* source = CreateAndAddDataSource(
      browser_context, viewer_url.host(), IDR_AI_CHAT_LEO_WORKSPACE_VIEW_HTML);

  // Viewer-specific CSP overrides: allow 'unsafe-inline' styles for dynamic
  // content, allow framing itself for file display, allow being framed by
  // workspace, and allow service worker.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      "style-src 'self' 'unsafe-inline' chrome-untrusted://resources;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc, "frame-src 'self';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameAncestors,
      absl::StrFormat("frame-ancestors %s;", UntrustedOrigin(workspace_host)));
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::WorkerSrc, "worker-src 'self';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc, "img-src 'self';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::MediaSrc, "media-src 'self';");
}

void CreateAndAddWorkspaceDataSource(content::BrowserContext* browser_context,
                                     const GURL& url) {
  content::WebUIDataSource* source = CreateAndAddDataSource(
      browser_context, url.host(), IDR_AI_CHAT_LEO_WORKSPACE_HTML);

  // Workspace-specific CSP overrides: allow inline styles and framing the
  // viewer origin.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      "style-src 'self' 'unsafe-inline' chrome-untrusted://resources;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc,
      absl::StrFormat("frame-src %s;",
                      UntrustedOrigin(base::StrCat(
                          {kAIChatLeoWorkspaceViewUIHostPrefix, url.host()}))));

  // Allow being framed by the AI Chat page for the workspace file lightbox.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameAncestors,
      absl::StrFormat("frame-ancestors %s;", kAIChatUIURL));
}

// Registers the service worker from the browser since chrome-untrusted://
// origins cannot call navigator.serviceWorker.register().
void RegisterViewerServiceWorker(content::BrowserContext* browser_context,
                                 const GURL& viewer_url,
                                 const GURL& workspace_url) {
  content::ServiceWorkerContext* service_worker_context =
      browser_context->GetStoragePartitionForUrl(viewer_url)
          ->GetServiceWorkerContext();
  if (!service_worker_context) {
    return;
  }

  // Storage is partitioned under the workspace's site since the viewer is
  // cross-site framed by the workspace.
  const blink::StorageKey storage_key = blink::StorageKey::Create(
      url::Origin::Create(viewer_url), net::SchemefulSite(workspace_url),
      blink::mojom::AncestorChainBit::kCrossSite);

  blink::mojom::ServiceWorkerRegistrationOptions options(
      viewer_url, blink::mojom::ScriptType::kModule,
      blink::mojom::ServiceWorkerUpdateViaCache::kNone);
  service_worker_context->RegisterServiceWorker(
      viewer_url.Resolve(kViewerServiceWorkerScript), storage_key, options,
      content::GlobalRenderFrameHostId(),
      base::BindOnce(
          [](const GURL& viewer_url, blink::ServiceWorkerStatusCode status) {
            if (status != blink::ServiceWorkerStatusCode::kOk) {
              LOG(ERROR) << "Leo workspace viewer service worker registration "
                            "failed for "
                         << viewer_url << ": "
                         << blink::ServiceWorkerStatusToString(status);
            }
          },
          viewer_url));
}

}  // namespace

bool LeoWorkspaceUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return base::FeatureList::IsEnabled(features::kAIChatWorkspaceTools) &&
         IsAIChatEnabled(user_prefs::UserPrefs::Get(browser_context));
}

bool LeoWorkspaceUIConfig::ShouldHandleSubdomains() const {
  return true;
}

bool LeoWorkspaceUIConfig::ShouldHandleURL(const GURL& url) {
  return IsAIChatLeoWorkspaceHost(url.host()) ||
         IsAIChatLeoWorkspaceViewHost(url.host());
}

std::unique_ptr<content::WebUIController>
LeoWorkspaceUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                            const GURL& url) {
  if (IsAIChatLeoWorkspaceViewHost(url.host())) {
    return std::make_unique<LeoWorkspaceViewUI>(web_ui, url);
  }
  return std::make_unique<LeoWorkspaceUI>(web_ui, url);
}

LeoWorkspaceUIConfig::LeoWorkspaceUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme,
                  kAIChatLeoWorkspaceUIHost) {}

LeoWorkspaceUIConfig::~LeoWorkspaceUIConfig() = default;

void LeoWorkspaceUIConfig::RegisterURLDataSource(
    content::BrowserContext* browser_context) {}

bool LeoWorkspaceUIConfig::ShouldInterceptNavigationsWithServiceWorker() {
  return true;
}

LeoWorkspaceUI::LeoWorkspaceUI(content::WebUI* web_ui, const GURL& url)
    : ui::UntrustedWebUIController(web_ui) {
  CHECK(IsAIChatLeoWorkspaceHost(url.host()));
  content::BrowserContext* browser_context =
      web_ui->GetWebContents()->GetBrowserContext();
  CreateAndAddWorkspaceDataSource(browser_context, url);
}

LeoWorkspaceUI::~LeoWorkspaceUI() = default;

LeoWorkspaceViewUI::LeoWorkspaceViewUI(content::WebUI* web_ui, const GURL& url)
    : ui::UntrustedWebUIController(web_ui) {
  CHECK(IsAIChatLeoWorkspaceViewHost(url.host()));
  content::BrowserContext* browser_context =
      web_ui->GetWebContents()->GetBrowserContext();
  const GURL viewer_url(UntrustedOrigin(url.host()) + "/");
  const std::string_view workspace_host = url.host().substr(
      std::string_view(kAIChatLeoWorkspaceViewUIHostPrefix).size());
  CreateAndAddViewerDataSource(browser_context, viewer_url, workspace_host);
  RegisterViewerServiceWorker(browser_context, viewer_url,
                              GURL(UntrustedOrigin(workspace_host) + "/"));
}

LeoWorkspaceViewUI::~LeoWorkspaceViewUI() = default;

}  // namespace ai_chat
