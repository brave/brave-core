/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/psst/psst_internals_ui.h"

#include <utility>

#include "brave/browser/psst/psst_log_service_factory.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/psst/resources/grit/brave_psst_internals_generated_map.h"
#include "brave/components/psst/resources/grit/brave_psst_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"

namespace psst {

bool PsstInternalsUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return PsstLogServiceFactory::GetForBrowserContext(browser_context) !=
         nullptr;
}

PsstInternalsUI::PsstInternalsUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui),
      handler_(
          PsstLogServiceFactory::GetForProfile(Profile::FromWebUI(web_ui))) {
  CreateAndAddWebUIDataSource(web_ui, kPsstInternalsHost,
                              kBravePsstInternalsGenerated,
                              IDR_PSST_INTERNALS_HTML);
}

PsstInternalsUI::~PsstInternalsUI() = default;

void PsstInternalsUI::BindInterface(
    mojo::PendingReceiver<mojom::PsstInternalsHandler> receiver) {
  handler_.BindInterface(std::move(receiver));
}

WEB_UI_CONTROLLER_TYPE_IMPL(PsstInternalsUI)

}  // namespace psst
