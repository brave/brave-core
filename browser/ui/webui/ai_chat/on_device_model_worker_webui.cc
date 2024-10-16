// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/on_device_model_worker_webui.h"

#include <utility>

#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ui/side_panel/ai_chat/ai_chat_side_panel_utils.h"
#include "brave/browser/ui/webui/ai_chat/ai_chat_ui_page_handler.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/ai_chat/resources/on_device_model_worker/grit/on_device_model_worker_generated_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/localization_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "components/grit/brave_components_resources.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"

UntrustedOnDeviceModelWorkerWebUI::UntrustedOnDeviceModelWorkerWebUI(content::WebUI* web_ui)
    : ui::UntrustedWebUIController(web_ui),
      profile_(Profile::FromWebUI(web_ui)) {
  DCHECK(profile_);
  DCHECK(profile_->IsRegularProfile());

  // Create a URLDataSource and add resources.
  content::WebUIDataSource* untrusted_source =
      content::WebUIDataSource::CreateAndAdd(
          web_ui->GetWebContents()->GetBrowserContext(), kOnDeviceModelWorkerURL);

  webui::SetupWebUIDataSource(
      untrusted_source,
      base::make_span(kOnDeviceModelWorkerGenerated, kOnDeviceModelWorkerGeneratedSize),
      IDR_ON_DEVICE_MODEL_WORKER_HTML);

  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src 'self' 'wasm-eval' 'unsafe-inline' https: chrome-untrusted://resources;");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ConnectSrc,
      "connect-src 'self' https: chrome-untrusted://resources;");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      "style-src 'self' 'unsafe-inline' chrome-untrusted://resources;");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      "img-src 'self' blob: chrome-untrusted://resources;");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FontSrc,
      "font-src 'self' data: chrome-untrusted://resources;");

  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::TrustedTypes, "");
}

UntrustedOnDeviceModelWorkerWebUI::~UntrustedOnDeviceModelWorkerWebUI() = default;

void UntrustedOnDeviceModelWorkerWebUI::BindInterface(
    mojo::PendingReceiver<ai_chat::mojom::Service> receiver) {
  ai_chat::AIChatServiceFactory::GetForBrowserContext(profile_)->Bind(
      std::move(receiver));
}

bool UntrustedOnDeviceModelWorkerWebUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return ai_chat::IsAIChatEnabled(
             user_prefs::UserPrefs::Get(browser_context)) &&
         Profile::FromBrowserContext(browser_context)->IsRegularProfile();
}

std::unique_ptr<content::WebUIController>
UntrustedOnDeviceModelWorkerWebUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                             const GURL& url) {
  return std::make_unique<UntrustedOnDeviceModelWorkerWebUI>(web_ui);
}

UntrustedOnDeviceModelWorkerWebUIConfig::UntrustedOnDeviceModelWorkerWebUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme, kOnDeviceModelWorkerHost) {}

WEB_UI_CONTROLLER_TYPE_IMPL(UntrustedOnDeviceModelWorkerWebUI)
