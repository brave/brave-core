/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/psst/brave_psst_dialog_ui.h"

#include <memory>

#include "base/check.h"
#include "brave/browser/ui/brave_browser_window.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/browser/ui/webui/psst/brave_psst_consent_helper_handler.h"
#include "brave/components/psst/browser/core/psst_consent_dialog.mojom.h"
#include "brave/components/psst/resources/grit/brave_psst_dialog_generated.h"
#include "brave/components/psst/resources/grit/brave_psst_dialog_generated_map.h"
#include "brave/components/psst/resources/grit/brave_psst_resources.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser_finder.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/base/l10n/l10n_util.h"

using content::WebUIMessageHandler;
namespace psst {
namespace {
void AddLocalizedStrings(content::WebUIDataSource* source) {
  DCHECK(source);
  static constexpr webui::LocalizedString kLocalizedStrings[] = {
      {"bravePsstDialogTitle", IDS_PSST_CONSENT_DIALOG_TITLE},
      {"bravePsstDialogText", IDS_PSST_CONSENT_DIALOG_BODY},
      {"bravePsstDialogOptionsTitle", IDS_PSST_CONSENT_DIALOG_OPTIONS_TITLE},
      {"bravePsstDialogOkBtn", IDS_PSST_COMPLETE_CONSENT_DIALOG_OK},
      {"bravePsstDialogReportFailedBtn", IDS_PSST_COMPLETE_CONSENT_DIALOG_REPORT_FAILED},
      {"bravePsstDialogCloseBtn", IDS_PSST_COMPLETE_CONSENT_DIALOG_CLOSE},
      {"bravePsstDialogCancelBtn", IDS_PSST_COMPLETE_CONSENT_DIALOG_CANCEL},
  };
  for (const auto& [name, id] : kLocalizedStrings) {
    source->AddString(name, l10n_util::GetStringUTF16(id));
  }
}
}  // namespace

BravePsstDialogUI::BravePsstDialogUI(content::WebUI* web_ui)
    : MojoWebDialogUI(web_ui), browser_(chrome::FindLastActive()) {
  auto* source = CreateAndAddWebUIDataSource(web_ui, kBravePsstHost,
                                             kBravePsstDialogGenerated,
                                             IDR_BRAVE_PSST_DIALOG_HTML);
  AddLocalizedStrings(source);
}

BravePsstDialogUI::~BravePsstDialogUI() = default;

void BravePsstDialogUI::BindInterface(
    mojo::PendingReceiver<psst_consent_dialog::mojom::PsstConsentFactory>
        receiver) {
  psst_consent_factory_receiver_.reset();
  psst_consent_factory_receiver_.Bind(std::move(receiver));
}

void BravePsstDialogUI::CreatePsstConsentHandler(
    ::mojo::PendingReceiver<psst_consent_dialog::mojom::PsstConsentHelper>
        psst_consent_helper,
    ::mojo::PendingRemote<psst_consent_dialog::mojom::PsstConsentDialog>
        psst_consent_dialog) {
  psst_consent_handler_ = std::make_unique<BravePsstConsentHelperHandler>(
      browser_->tab_strip_model(), this, std::move(psst_consent_helper),
      std::move(psst_consent_dialog));

  // data_handler_ = std::make_unique<ShieldsPanelDataHandler>(
  //     std::move(data_handler_receiver), this, browser_->tab_strip_model());
}

WEB_UI_CONTROLLER_TYPE_IMPL(BravePsstDialogUI)
}  // namespace psst
