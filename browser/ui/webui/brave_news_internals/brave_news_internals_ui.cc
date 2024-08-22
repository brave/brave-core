// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_news_internals/brave_news_internals_ui.h"

#include <string>
#include <utility>

#include "brave/browser/brave_news/brave_news_controller_factory.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/brave_news/browser/brave_news_controller.h"
#include "brave/components/brave_news/browser/resources/grit/brave_news_internals_generated_map.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"

BraveNewsInternalsUI::BraveNewsInternalsUI(content::WebUI* web_ui,
                                           const std::string& host)
    : content::WebUIController(web_ui) {
  auto* source = CreateAndAddWebUIDataSource(
      web_ui, host, kBraveNewsInternalsGenerated,
      kBraveNewsInternalsGeneratedSize, IDR_BRAVE_NEWS_INTERNALS_HTML);
  DCHECK(source);
}

BraveNewsInternalsUI::~BraveNewsInternalsUI() = default;
WEB_UI_CONTROLLER_TYPE_IMPL(BraveNewsInternalsUI)

void BraveNewsInternalsUI::BindInterface(
    mojo::PendingReceiver<brave_news::mojom::BraveNewsController> receiver) {
  auto* profile = Profile::FromWebUI(web_ui());
  auto* controller =
      brave_news::BraveNewsControllerFactory::GetForBrowserContext(profile);
  if (!controller) {
    return;
  }

  controller->Bind(std::move(receiver));
}

void BraveNewsInternalsUI::BindInterface(
    mojo::PendingReceiver<brave_news::mojom::BraveNewsInternals> receiver) {
  auto* profile = Profile::FromWebUI(web_ui());
  auto* controller =
      brave_news::BraveNewsControllerFactory::GetForBrowserContext(profile);
  if (!controller) {
    return;
  }

  controller->Bind(std::move(receiver));
}
