/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/tor_internals_ui.h"

#include <utility>

#include "brave/components/tor/resources/grit/tor_internals_generated_map.h"
#include "brave/components/tor/resources/grit/tor_resources.h"
#include "brave/components/tor/tor_launcher_factory.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"

TorInternalsDOMHandler::TorInternalsDOMHandler() : weak_ptr_factory_{this} {}

TorInternalsDOMHandler::~TorInternalsDOMHandler() {}

void TorInternalsDOMHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "tor_internals.getTorGeneralInfo",
      base::BindRepeating(&TorInternalsDOMHandler::HandleGetTorGeneralInfo,
                          base::Unretained(this)));
}

void TorInternalsDOMHandler::HandleGetTorGeneralInfo(const base::ListValue* args) {
  DCHECK_EQ(args->GetSize(), 0U);
  if (!web_ui()->CanCallJavascript())
    return;
  TorLauncherFactory* tlf = TorLauncherFactory::GetInstance();
  if (!tlf)
    return;
  base::Value info(base::Value::Type::DICTIONARY);

  info.SetStringKey("torVersion", tlf->GetTorVersion());
  info.SetIntKey("torPid", static_cast<int>(tlf->GetTorPid()));
  info.SetStringKey("torProxyURI", tlf->GetTorProxyURI());
  info.SetBoolKey("isTorConnected", tlf->IsTorConnected());
  web_ui()->CallJavascriptFunctionUnsafe("tor_internals.onGetTorGeneralInfo",
                                         std::move(info));
}

TorInternalsUI::TorInternalsUI(content::WebUI* web_ui, const std::string& name)
    : BasicUI(web_ui,
              name,
              kTorInternalsGenerated,
              kTorInternalsGeneratedSize,
              IDR_TOR_INTERNALS_HTML) {
  web_ui->AddMessageHandler(std::make_unique<TorInternalsDOMHandler>());
}

TorInternalsUI::~TorInternalsUI() {}
