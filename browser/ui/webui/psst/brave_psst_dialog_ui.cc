/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/psst/brave_psst_dialog_ui.h"

#include <cstddef>
#include <memory>
#include <utility>

#include "base/check.h"
#include "brave/browser/psst/psst_ui_desktop_presenter.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/browser/ui/webui/psst/brave_psst_dialog_handler.h"
#include "brave/components/psst/resources/grit/brave_psst_dialog_generated_map.h"
#include "brave/components/psst/resources/grit/brave_psst_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface_iterator.h"
#include "components/grit/brave_components_webui_strings.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"

using content::WebUIMessageHandler;

namespace psst {

BravePsstDialogUI::BravePsstDialogUI(content::WebUI* web_ui)
    : MojoWebDialogUI(web_ui) {
  auto* source = CreateAndAddWebUIDataSource(web_ui, kBravePsstHost,
                                             kBravePsstDialogGenerated,
                                             IDR_BRAVE_PSST_DIALOG_HTML);
  source->AddLocalizedStrings(webui::kPsstStrings);
}

BravePsstDialogUI::~BravePsstDialogUI() = default;

void BravePsstDialogUI::BindInterface(
    mojo::PendingReceiver<psst::mojom::PsstConsentFactory> receiver) {
  psst_consent_factory_receiver_.reset();
  psst_consent_factory_receiver_.Bind(std::move(receiver));
}

void BravePsstDialogUI::CreatePsstConsentHandler(
    ::mojo::PendingReceiver<psst::mojom::PsstConsentHelper> psst_consent_helper,
    ::mojo::PendingRemote<psst::mojom::PsstConsentDialog> psst_consent_dialog) {
  auto* delegate =
      PsstUiDesktopPresenter::PsstUiDesktopDelegate::GetDelegateFromWebContents(
          web_ui()->GetWebContents());
  CHECK(delegate);
  auto* initiator_contents = delegate->GetInitiatorWebContents();
  CHECK(initiator_contents);

  auto* tab_interface =
      tabs::TabInterface::MaybeGetFromContents(initiator_contents);
  CHECK(tab_interface);

  TabStripModel* tab_strip_model =
      tab_interface->GetBrowserWindowInterface()->GetTabStripModel();
  CHECK(tab_strip_model);

  psst_consent_handler_ = std::make_unique<BravePsstDialogHandler>(
      tab_strip_model, this, std::move(psst_consent_helper),
      std::move(psst_consent_dialog));
}

WEB_UI_CONTROLLER_TYPE_IMPL(BravePsstDialogUI)

}  // namespace psst
