// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/leo_workspace_ui.h"

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "brave/components/ai_chat/content/browser/workspace_content_source.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/resources/grit/ai_chat_ui_generated_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "components/grit/brave_components_resources.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "ui/webui/webui_util.h"
#include "url/gurl.h"
#include "url/origin.h"

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

  // Serves each workspace's folder under /<uuid>/files/; everything else on
  // this host is the tool page and its bundle.
  source->SetRequestFilter(
      base::BindRepeating(&ShouldHandleWorkspaceFileRequest),
      base::BindRepeating(&HandleWorkspaceFileRequest,
                          browser_context->GetWeakPtr()));

  // Per-data-source, so the tool page and the previews served alongside it
  // share one policy; it is written for the previews and is no boundary between
  // them. See workspace_content_source.h.
  //
  // 'unsafe-inline'/'unsafe-eval' cost nothing here: same-origin .js served out
  // of the folder already runs either way.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::DefaultSrc, "default-src 'self';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src 'self' 'unsafe-inline' 'unsafe-eval' "
      "chrome-untrusted://resources;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      "style-src 'self' 'unsafe-inline' chrome-untrusted://resources;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc, "img-src 'self' data: blob:;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FontSrc, "font-src 'self' data:;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::MediaSrc,
      "media-src 'self' data: blob:;");
  // No network at all, so a preview cannot send a folder's contents anywhere.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ConnectSrc, "connect-src 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ObjectSrc, "object-src 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc, "frame-src 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::WorkerSrc, "worker-src 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FormAction, "form-action 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::BaseURI, "base-uri 'none';");

  // Previews are shown inside Leo, which also makes the headless tool page
  // framable. Serialising as an origin drops the constants' trailing slash,
  // which Blink would otherwise report as an ignored path.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameAncestors,
      absl::StrFormat(
          "frame-ancestors %s %s;",
          url::Origin::Create(GURL(kAIChatUIURL)).Serialize(),
          url::Origin::Create(GURL(kAIChatUntrustedConversationUIURL))
              .Serialize()));
}

LeoWorkspaceUI::~LeoWorkspaceUI() = default;

}  // namespace ai_chat
