// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/commands_ui.h"

#include <utility>

#include "base/feature_list.h"
#include "brave/browser/ui/views/commands/accelerator_service.h"
#include "brave/browser/ui/views/commands/accelerator_service_factory.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/commands/browser/resources/grit/commands_generated_map.h"
#include "brave/components/commands/common/features.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller.h"

namespace commands {

CommandsUI::CommandsUI(content::WebUI* web_ui, const std::string& name)
    : content::WebUIController(web_ui) {
  DCHECK(base::FeatureList::IsEnabled(features::kBraveCommands));
  CreateAndAddWebUIDataSource(web_ui, name, kCommandsGenerated,
                              kCommandsGeneratedSize, IDR_COMMANDS_HTML);
}

CommandsUI::~CommandsUI() = default;

void CommandsUI::BindInterface(
    mojo::PendingReceiver<mojom::CommandsService> pending_receiver) {
  commands::AcceleratorServiceFactory::GetForContext(
      web_ui()->GetWebContents()->GetBrowserContext())
      ->BindInterface(std::move(pending_receiver));
}

WEB_UI_CONTROLLER_TYPE_IMPL(CommandsUI)

}  // namespace commands
