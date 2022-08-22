/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_federated/federated_internals_ui.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/values.h"
#include "brave/browser/resources/federated_internals/grit/federated_internals_resources.h"
#include "brave/browser/resources/federated_internals/grit/federated_internals_resources_map.h"
#include "brave/browser/ui/webui/brave_federated/federated_internals_page_handler.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"

namespace brave_federated {

FederatedInternalsUI::FederatedInternalsUI(content::WebUI* web_ui)
    : MojoWebUIController(web_ui) {
  profile_ = Profile::FromWebUI(web_ui);

  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(kFederatedInternalsHost);

  webui::SetupWebUIDataSource(source,
                              base::make_span(kFederatedInternalsResources,
                                              kFederatedInternalsResourcesSize),
                              IDR_FEDERATED_INTERNALS_FEDERATED_INTERNALS_HTML);

  content::BrowserContext* browser_context =
      web_ui->GetWebContents()->GetBrowserContext();
  content::WebUIDataSource::Add(browser_context, source);
}

FederatedInternalsUI::~FederatedInternalsUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(FederatedInternalsUI)

void FederatedInternalsUI::BindInterface(
    mojo::PendingReceiver<federated_internals::mojom::PageHandlerFactory>
        receiver) {
  federated_internals_page_factory_receiver_.reset();
  federated_internals_page_factory_receiver_.Bind(std::move(receiver));
}

void FederatedInternalsUI::CreatePageHandler(
    mojo::PendingRemote<federated_internals::mojom::Page> page,
    mojo::PendingReceiver<federated_internals::mojom::PageHandler>
        receiver) {
  federated_internals_page_handler_ =
      std::make_unique<FederatedInternalsPageHandler>(
          std::move(receiver), std::move(page), profile_);
}

}  // namespace brave_federated
