/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_player_ui.h"

#include <string>

#include "base/strings/strcat.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/brave_player/browser/resources/grit/brave_player_generated_map.h"
#include "brave/components/brave_player/common/url_constants.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/bindings_policy.h"
#include "content/public/common/url_constants.h"
#include "ui/webui/untrusted_web_ui_controller.h"

namespace {
class UntrustedBravePlayerEmbedUI : public ui::UntrustedWebUIController {
 public:
  explicit UntrustedBravePlayerEmbedUI(content::WebUI* web_ui)
      : UntrustedWebUIController(web_ui) {
    auto* source = CreateAndAddWebUIDataSource(
        web_ui, brave_player::kBravePlayerEmbedURL, kBravePlayerGenerated,
        kBravePlayerGeneratedSize, IDR_BRAVE_PLAYER_EMBED_HTML);
    source->AddFrameAncestor(GURL(brave_player::kBravePlayerURL));

    source->OverrideContentSecurityPolicy(
        network::mojom::CSPDirectiveName::StyleSrc,
        std::string("style-src chrome://resources "
                    "chrome://brave-resources 'unsafe-inline';"));
    source->OverrideContentSecurityPolicy(
        network::mojom::CSPDirectiveName::FrameSrc,
        base::StrCat({"frame-src ", "https://www.youtube-nocookie.com", ";"}));
  }

  UntrustedBravePlayerEmbedUI(const UntrustedBravePlayerEmbedUI&) = delete;
  UntrustedBravePlayerEmbedUI& operator=(const UntrustedBravePlayerEmbedUI&) =
      delete;
  ~UntrustedBravePlayerEmbedUI() override = default;
};

}  // namespace

BravePlayerUI::BravePlayerUI(content::WebUI* web_ui) : WebUIController(web_ui) {
  // In order to make requests to external services, we should clear bindings
  // so that the webui is not communicating with the browser process. Otherwise
  // the requests won't be allowed.
  web_ui->SetBindings(content::BINDINGS_POLICY_NONE);

  auto* source = CreateAndAddWebUIDataSource(
      web_ui, brave_player::kBravePlayerHost, kBravePlayerGenerated,
      kBravePlayerGeneratedSize, IDR_BRAVE_PLAYER_HTML);
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc,
      base::StrCat({"frame-src ", brave_player::kBravePlayerEmbedURL, ";"}));
  web_ui->AddRequestableScheme(content::kChromeUIUntrustedScheme);
}

BravePlayerUI::~BravePlayerUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(BravePlayerUI)

UntrustedBravePlayerEmbedUIConfig::UntrustedBravePlayerEmbedUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme,
                  brave_player::kBravePlayerEmbedHost) {}

std::unique_ptr<content::WebUIController>
UntrustedBravePlayerEmbedUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                                         const GURL& url) {
  return std::make_unique<UntrustedBravePlayerEmbedUI>(web_ui);
}
