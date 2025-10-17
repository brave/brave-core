// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/code_sandbox_ui.h"

#include <string>

#include "base/memory/ref_counted_memory.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/code_sandbox_script_storage.h"
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
#include "url/gurl.h"

namespace ai_chat {

// CodeSandboxUIConfig implementation
bool CodeSandboxUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  auto* profile = Profile::FromBrowserContext(browser_context);
  return IsAIChatEnabled(user_prefs::UserPrefs::Get(browser_context)) &&
         profile->IsOffTheRecord() &&
         profile->GetOTRProfileID().IsCodeSandbox();
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

  source->SetDefaultResource(IDR_AI_CHAT_CODE_SANDBOX_UI_HTML);

  source->SetRequestFilter(
      base::BindRepeating(&CodeSandboxUI::ShouldHandleRequest),
      base::BindRepeating(&CodeSandboxUI::HandleScriptRequest,
                          browser_context->GetWeakPtr()));

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::DefaultSrc, "default-src 'none';");

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc, "script-src 'self';");

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
}

bool CodeSandboxUI::ShouldHandleRequest(const std::string& path) {
  return path.ends_with("/script.js") || path == "script.js" || path.empty();
}

void CodeSandboxUI::HandleScriptRequest(
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
    auto script = ai_chat_service->code_sandbox_script_storage()->ConsumeScript(
        request_id);
    if (script) {
      script_content = std::move(*script);
    }
  }

  auto script =
      base::MakeRefCounted<base::RefCountedString>(std::move(script_content));
  std::move(callback).Run(script.get());
}

CodeSandboxUI::~CodeSandboxUI() = default;

}  // namespace ai_chat
