// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/local_ai_internals/local_ai_internals_ui.h"

#include <utility>

#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"

namespace local_ai {

LocalAIInternalsUI::LocalAIInternalsUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  auto* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource* source =
      content::WebUIDataSource::CreateAndAdd(profile, kLocalAIInternalsHost);
  web_ui->AddRequestableScheme(content::kChromeUIUntrustedScheme);

  // Allow embedding the untrusted candle WASM iframes
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc,
      std::string("frame-src ") + kUntrustedCandleBertWasmURL + " " +
          kUntrustedCandleEmbeddingGemmaWasmURL + " " +
          kUntrustedCandlePhiWasmURL + ";");

  // Allow loading resources from chrome:// and chrome-untrusted://
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src chrome://resources chrome-untrusted://resources 'self' "
      "'wasm-unsafe-eval';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ConnectSrc,
      "connect-src chrome://resources chrome-untrusted://resources 'self' "
      "'wasm-unsafe-eval';");

  // Serve the HTML page
  source->AddResourcePath("", IDR_LOCAL_AI_INTERNALS_HTML);
  source->SetDefaultResource(IDR_LOCAL_AI_INTERNALS_HTML);

  source->UseStringsJs();
}

LocalAIInternalsUI::~LocalAIInternalsUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(LocalAIInternalsUI)

///////////////////////////////////////////////////////////////////////////////

LocalAIInternalsUIConfig::LocalAIInternalsUIConfig()
    : DefaultWebUIConfig(content::kChromeUIScheme, kLocalAIInternalsHost) {}

}  // namespace local_ai
