// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_news_inspect/brave_news_inspect_ui.h"

#include "brave/browser/brave_news/brave_news_controller_factory.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/brave_news/browser/brave_news_controller.h"
#include "brave/components/brave_news/browser/resources/grit/brave_news_inspect_generated_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"

BraveNewsInspectUI::BraveNewsInspectUI(content::WebUI* web_ui,
                                       const std::string& host)
    : content::WebUIController(web_ui) {
  auto* source = CreateAndAddWebUIDataSource(
      web_ui, host, kBraveNewsInspectGenerated, kBraveNewsInspectGeneratedSize,
      IDR_BRAVE_NEWS_INSPECT_HTML);
  DCHECK(source);
}

BraveNewsInspectUI::~BraveNewsInspectUI() = default;
WEB_UI_CONTROLLER_TYPE_IMPL(BraveNewsInspectUI)

void BraveNewsInspectUI::BindInterface(
    mojo::PendingReceiver<brave_news::mojom::BraveNewsController> receiver) {
  auto* profile = Profile::FromWebUI(web_ui());
  auto* controller =
      brave_news::BraveNewsControllerFactory::GetForContext(profile);
  if (!controller) {
    return;
  }

  controller->Bind(std::move(receiver));
}
