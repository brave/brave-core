// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/leo_workspace_ui.h"

#include <string>
#include <string_view>

#include "base/check.h"
#include "base/feature_list.h"
#include "base/strings/strcat.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/resources/grit/ai_chat_ui_generated_map.h"
#include "components/grit/brave_components_resources.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "ui/webui/webui_util.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace ai_chat {

namespace {

std::string UntrustedOrigin(std::string_view host) {
  return base::StrCat(
      {content::kChromeUIUntrustedScheme, url::kStandardSchemeSeparator, host});
}

// Adds the data source for the document served from `url`'s own host, locked
// down to its own first-party bundle plus whatever `frame_src` and
// `frame_ancestors` (full CSP directive values) allow.
//
// Untrusted data sources are looked up by the origin they serve, so every host
// under the workspace host needs one of its own: URLDataManagerBackend keys
// chrome-untrusted:// sources on "chrome-untrusted://<host>/".
void AddViewerDataSource(content::WebUI* web_ui,
                             const GURL& url,
                             int default_resource,
                             std::string_view frame_src,
                             std::string_view frame_ancestors) {
  auto* source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(),
      UntrustedOrigin(url.host()) + "/");

  webui::SetupWebUIDataSource(source, kAiChatUiGenerated, default_resource);

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
      network::mojom::CSPDirectiveName::FrameSrc,
      base::StrCat({"frame-src ", frame_src, ";"}));
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameAncestors,
      base::StrCat({"frame-ancestors ", frame_ancestors, ";"}));
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::WorkerSrc, "worker-src 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FormAction, "form-action 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::BaseURI, "base-uri 'none';");
}

}  // namespace

bool LeoWorkspaceUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return IsAIChatEnabled(user_prefs::UserPrefs::Get(browser_context)) &&
         base::FeatureList::IsEnabled(features::kAIChatWorkspaceTools);
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
  // ShouldHandleURL() has already rejected every host that is neither a
  // workspace nor a viewer.
  if (IsAIChatLeoWorkspaceViewHost(url.host())) {
    return std::make_unique<LeoWorkspaceViewUI>(web_ui, url);
  }
  return std::make_unique<LeoWorkspaceUI>(web_ui, url);
}

LeoWorkspaceUIConfig::LeoWorkspaceUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme,
                  kAIChatLeoWorkspaceUIHost) {}

LeoWorkspaceUIConfig::~LeoWorkspaceUIConfig() = default;

LeoWorkspaceUI::LeoWorkspaceUI(content::WebUI* web_ui, const GURL& url)
    : ui::UntrustedWebUIController(web_ui) {
  CHECK(IsAIChatLeoWorkspaceHost(url.host()));
  // The FileSystemDirectoryHandle the page operates on is delivered
  // out-of-band (launchQueue), so it needs no network access of its own.
  AddViewerDataSource(
      web_ui, url, IDR_AI_CHAT_LEO_WORKSPACE_HTML,
      /*frame_src=*/
      UntrustedOrigin(
          base::StrCat({kAIChatLeoWorkspaceViewUIHostPrefix, url.host()})),
      /*frame_ancestors=*/"'none'");
}

LeoWorkspaceUI::~LeoWorkspaceUI() = default;

LeoWorkspaceViewUI::LeoWorkspaceViewUI(content::WebUI* web_ui, const GURL& url)
    : ui::UntrustedWebUIController(web_ui) {
  CHECK(IsAIChatLeoWorkspaceViewHost(url.host()));
  // Restricting the framer to this viewer's own workspace stops one workspace
  // presenting another's contents as its own.
  const std::string_view workspace_host = url.host().substr(
      std::string_view(kAIChatLeoWorkspaceViewUIHostPrefix).size());
  AddViewerDataSource(web_ui, url, IDR_AI_CHAT_LEO_WORKSPACE_VIEW_HTML,
                          /*frame_src=*/"'none'",
                          /*frame_ancestors=*/UntrustedOrigin(workspace_host));
}

LeoWorkspaceViewUI::~LeoWorkspaceViewUI() = default;

}  // namespace ai_chat
