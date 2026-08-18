// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/leo_workspace_ui.h"

#include "base/feature_list.h"
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

namespace ai_chat {

bool LeoWorkspaceUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return IsAIChatEnabled(user_prefs::UserPrefs::Get(browser_context)) &&
         base::FeatureList::IsEnabled(features::kAIChatWorkspaceTools);
}

std::unique_ptr<content::WebUIController>
LeoWorkspaceUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                            const GURL& url) {
  return std::make_unique<LeoWorkspaceUI>(web_ui);
}

LeoWorkspaceUIConfig::LeoWorkspaceUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme,
                  kAIChatLeoWorkspaceUIHost) {}

LeoWorkspaceUIConfig::~LeoWorkspaceUIConfig() = default;

LeoWorkspaceUI::LeoWorkspaceUI(content::WebUI* web_ui)
    : ui::UntrustedWebUIController(web_ui) {
  auto* browser_context = web_ui->GetWebContents()->GetBrowserContext();
  auto* source = content::WebUIDataSource::CreateAndAdd(
      browser_context, kAIChatLeoWorkspaceUIURL);

  webui::SetupWebUIDataSource(source, kAiChatUiGenerated,
                              IDR_AI_CHAT_LEO_WORKSPACE_HTML);

  // This page runs its own first-party module bundle only. No network, no
  // frames, no embedding by other pages. The FileSystemDirectoryHandle it will
  // operate on is delivered out-of-band (launchQueue), not fetched.
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
}

LeoWorkspaceUI::~LeoWorkspaceUI() = default;

}  // namespace ai_chat
