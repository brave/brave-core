/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/tor_internals_ui.h"

#include <utility>

#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/tor/resources/grit/tor_internals_generated_map.h"
#include "brave/components/tor/resources/grit/tor_resources.h"
#include "brave/components/tor/tor_launcher_factory.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_ui.h"

TorInternalsDOMHandler::TorInternalsDOMHandler()
    : tor_launcher_factory_(*TorLauncherFactory::GetInstance()),
      weak_ptr_factory_{this} {
  tor_launcher_factory_->AddObserver(this);
}

TorInternalsDOMHandler::~TorInternalsDOMHandler() {
  tor_launcher_factory_->RemoveObserver(this);
}

void TorInternalsDOMHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "tor_internals.getTorGeneralInfo",
      base::BindRepeating(&TorInternalsDOMHandler::HandleGetTorGeneralInfo,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "tor_internals.getTorLog",
      base::BindRepeating(&TorInternalsDOMHandler::HandleGetTorLog,
                          base::Unretained(this)));
}

void TorInternalsDOMHandler::HandleGetTorGeneralInfo(
    const base::Value::List& args) {
  DCHECK_EQ(args.size(), 0U);
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  base::Value::Dict info;

  info.Set("torVersion", tor_launcher_factory_->GetTorVersion());
  info.Set("torPid", static_cast<int>(tor_launcher_factory_->GetTorPid()));
  info.Set("torProxyURI", tor_launcher_factory_->GetTorProxyURI());
  info.Set("isTorConnected", tor_launcher_factory_->IsTorConnected());
  web_ui()->CallJavascriptFunctionUnsafe("tor_internals.onGetTorGeneralInfo",
                                         info);
}

void TorInternalsDOMHandler::HandleGetTorLog(const base::Value::List& args) {
  DCHECK_EQ(args.size(), 0U);
  if (!web_ui()->CanCallJavascript()) {
    return;
  }
  tor_launcher_factory_->GetTorLog(base::BindOnce(
      &TorInternalsDOMHandler::OnGetTorLog, weak_ptr_factory_.GetWeakPtr()));
}

void TorInternalsDOMHandler::OnGetTorLog(bool success, const std::string& log) {
  if (success) {
    web_ui()->CallJavascriptFunctionUnsafe("tor_internals.onGetTorLog",
                                           base::Value(log));
  }
}

void TorInternalsDOMHandler::OnTorCircuitEstablished(bool result) {
  web_ui()->CallJavascriptFunctionUnsafe(
      "tor_internals.onGetTorCircuitEstablished", base::Value(result));
}

void TorInternalsDOMHandler::OnTorControlEvent(const std::string& event) {
  web_ui()->CallJavascriptFunctionUnsafe("tor_internals.onGetTorControlEvent",
                                         base::Value(event));
}

void TorInternalsDOMHandler::OnTorLogUpdated() {
  tor_launcher_factory_->GetTorLog(base::BindOnce(
      &TorInternalsDOMHandler::OnGetTorLog, weak_ptr_factory_.GetWeakPtr()));
}

void TorInternalsDOMHandler::OnTorInitializing(const std::string& percentage,
                                               const std::string& message) {
  web_ui()->CallJavascriptFunctionUnsafe("tor_internals.onGetTorInitPercentage",
                                         base::Value(percentage));
}

TorInternalsUI::TorInternalsUI(content::WebUI* web_ui, const std::string& name)
    : WebUIController(web_ui) {
  CreateAndAddWebUIDataSource(web_ui, name, kTorInternalsGenerated,
                              IDR_TOR_INTERNALS_HTML);
  web_ui->AddMessageHandler(std::make_unique<TorInternalsDOMHandler>());
}

TorInternalsUI::~TorInternalsUI() = default;
