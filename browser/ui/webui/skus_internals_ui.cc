// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/skus_internals_ui.h"

#include <utility>

#include "base/notreached.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/skus/browser/resources/grit/skus_internals_generated_map.h"
#include "components/grit/brave_components_resources.h"

SkusInternalsUI::SkusInternalsUI(content::WebUI* web_ui,
                                 const std::string& name)
    : content::WebUIController(web_ui) {
  CreateAndAddWebUIDataSource(web_ui, name, kSkusInternalsGenerated,
                              kSkusInternalsGeneratedSize,
                              IDR_SKUS_INTERNALS_HTML);
}

SkusInternalsUI::~SkusInternalsUI() = default;

void SkusInternalsUI::BindInterface(
    mojo::PendingReceiver<skus::mojom::SkusInternals> pending_receiver) {
  if (skus_internals_receiver_.is_bound()) {
    skus_internals_receiver_.reset();
  }

  skus_internals_receiver_.Bind(std::move(pending_receiver));
}

void SkusInternalsUI::GetEventLog(GetEventLogCallback callback) {
  // TODO(simonhong): Ask log to SkusService
  NOTIMPLEMENTED_LOG_ONCE();
}

WEB_UI_CONTROLLER_TYPE_IMPL(SkusInternalsUI)
