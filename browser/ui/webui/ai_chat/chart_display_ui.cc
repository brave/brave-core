// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/chart_display_ui.h"

#include <string>

#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/resources/grit/ai_chat_ui_generated_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "ui/webui/webui_util.h"
#include "url/gurl.h"

namespace ai_chat {

// ChartDisplayUIConfig implementation
bool ChartDisplayUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return ai_chat::AIChatServiceFactory::GetForBrowserContext(browser_context) !=
         nullptr;
}

std::unique_ptr<content::WebUIController>
ChartDisplayUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                            const GURL& url) {
  return std::make_unique<ChartDisplayUI>(web_ui);
}

ChartDisplayUIConfig::ChartDisplayUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme,
                  kAIChatChartDisplayUIHost) {}

ChartDisplayUIConfig::~ChartDisplayUIConfig() = default;

ChartDisplayUI::ChartDisplayUI(content::WebUI* web_ui)
    : ui::UntrustedWebUIController(web_ui) {
  auto* browser_context = web_ui->GetWebContents()->GetBrowserContext();
  auto* source = content::WebUIDataSource::CreateAndAdd(
      browser_context, kAIChatChartDisplayUIURL);

  webui::SetupWebUIDataSource(source, kAiChatUiGenerated,
                              IDR_AI_CHAT_CHART_DISPLAY_HTML);

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src 'self' chrome-untrusted://resources;");

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      "style-src 'self' 'unsafe-inline' chrome-untrusted://resources;");

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ConnectSrc, "connect-src 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ObjectSrc, "object-src 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc, "frame-src 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameAncestors,
      absl::StrFormat("frame-ancestors %s %s;", kAIChatUIURL,
                      kAIChatUntrustedConversationUIURL));
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::WorkerSrc, "worker-src 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FormAction, "form-action 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::BaseURI, "base-uri 'none';");
}

ChartDisplayUI::~ChartDisplayUI() = default;

}  // namespace ai_chat
