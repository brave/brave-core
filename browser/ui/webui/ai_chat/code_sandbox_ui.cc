// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/code_sandbox_ui.h"

#include <string>

#include "base/base64.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/strcat.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "url/gurl.h"

namespace ai_chat {

// CodeSandboxUIConfig implementation
bool CodeSandboxUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  auto* profile = Profile::FromBrowserContext(browser_context);
  return IsAIChatEnabled(user_prefs::UserPrefs::Get(browser_context)) &&
         profile->IsOffTheRecord();
}

std::unique_ptr<content::WebUIController>
CodeSandboxUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                           const GURL& url) {
  return std::make_unique<CodeSandboxUI>(web_ui);
}

CodeSandboxUIConfig::CodeSandboxUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme, kAIChatCodeSandboxUIHost) {
}

CodeSandboxUIConfig::~CodeSandboxUIConfig() = default;

CodeSandboxUI::CodeSandboxUI(content::WebUI* web_ui)
    : ui::UntrustedWebUIController(web_ui) {
  auto* browser_context = web_ui->GetWebContents()->GetBrowserContext();
  auto* source = content::WebUIDataSource::CreateAndAdd(
      browser_context, kAIChatCodeSandboxUIURL);

  source->SetRequestFilter(
      base::BindRepeating(&CodeSandboxUI::ShouldHandleRequest),
      base::BindRepeating(&CodeSandboxUI::HandleRequest,
                          browser_context->GetWeakPtr()));

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::DefaultSrc, "default-src 'none';");

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src 'unsafe-inline';");

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ConnectSrc, "connect-src 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ObjectSrc, "object-src 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc, "frame-src data:;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameAncestors,
      "frame-ancestors 'self';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::WorkerSrc, "worker-src 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FormAction, "form-action 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::BaseURI, "base-uri 'none';");
}

bool CodeSandboxUI::ShouldHandleRequest(const std::string& path) {
  return true;
}

void CodeSandboxUI::HandleRequest(
    base::WeakPtr<content::BrowserContext> browser_context,
    const std::string& path,
    content::WebUIDataSource::GotDataCallback callback) {
  std::string request_id;
  auto slash_pos = path.find('/');
  if (slash_pos != std::string::npos) {
    request_id = path.substr(0, slash_pos);
  }

  std::string script_content;
  auto* original_profile =
      Profile::FromBrowserContext(browser_context.get())->GetOriginalProfile();
  auto* ai_chat_service =
      AIChatServiceFactory::GetForBrowserContext(original_profile);
  if (ai_chat_service) {
    auto script = ai_chat_service->ConsumeCodeExecutionToolScript(request_id);
    if (script) {
      script_content = std::move(*script);
    }
  }

  // Base64 encode the entire iframe HTML to avoid escaping issues
  std::string iframe_html =
      base::StrCat({"<html><head><meta charset='utf-8'></head><body><script>",
                    script_content, "</script></body></html>"});

  std::string html_content = base::StrCat(
      {"<!DOCTYPE html><html><head><meta charset=\"utf-8\"></head><body>"
       "<iframe sandbox=\"allow-scripts\" src=\"data:text/html;base64,",
       base::Base64Encode(iframe_html), "\"></iframe></body></html>"});

  auto result =
      base::MakeRefCounted<base::RefCountedString>(std::move(html_content));
  std::move(callback).Run(result.get());
}

CodeSandboxUI::~CodeSandboxUI() = default;

}  // namespace ai_chat
