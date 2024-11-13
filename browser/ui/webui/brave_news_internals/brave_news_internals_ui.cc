// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_news_internals/brave_news_internals_ui.h"

#include <string>
#include <utility>

#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/brave_news/browser/brave_news_controller.h"
#include "brave/components/brave_news/browser/resources/grit/brave_news_internals_generated_map.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "components/grit/brave_components_resources.h"

BraveNewsInternalsUI::BraveNewsInternalsUI(
    content::WebUI* web_ui,
    const std::string& host,
    brave_news::BraveNewsController* controller)
    : content::WebUIController(web_ui), controller_(controller) {
  auto* source = CreateAndAddWebUIDataSource(
      web_ui, host, kBraveNewsInternalsGenerated,
      kBraveNewsInternalsGeneratedSize, IDR_BRAVE_NEWS_INTERNALS_HTML);
  DCHECK(source);
}

BraveNewsInternalsUI::~BraveNewsInternalsUI() = default;
WEB_UI_CONTROLLER_TYPE_IMPL(BraveNewsInternalsUI)

void BraveNewsInternalsUI::BindInterface(
    mojo::PendingReceiver<brave_news::mojom::BraveNewsController> receiver) {
  if (!controller_) {
    return;
  }

  controller_->Bind(std::move(receiver));
}

void BraveNewsInternalsUI::BindInterface(
    mojo::PendingReceiver<brave_news::mojom::BraveNewsInternals> receiver) {
  if (!controller_) {
    return;
  }

  controller_->Bind(std::move(receiver));
}
