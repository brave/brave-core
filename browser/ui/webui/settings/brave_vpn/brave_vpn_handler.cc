/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_vpn/brave_vpn_handler.h"

#include <memory>

#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"

BraveVpnHandler::BraveVpnHandler(Profile* profile) : profile_(profile) {
  auto* service = brave_vpn::BraveVpnServiceFactory::GetForProfile(profile);
  if (service) {
    Observe(service);
  }
}

BraveVpnHandler::~BraveVpnHandler() = default;

void BraveVpnHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "registerWireguardService",
      base::BindRepeating(&BraveVpnHandler::HandleRegisterWireguardService,
                          base::Unretained(this)));
}

void BraveVpnHandler::HandleRegisterWireguardService(
    const base::Value::List& args) {}

void BraveVpnHandler::OnConnectionStateChanged(
    brave_vpn::mojom::ConnectionState state) {
  AllowJavascript();
  FireWebUIListener(
      "brave-vpn-state-change",
      base::Value(state == brave_vpn::mojom::ConnectionState::CONNECTED));
}

void BraveVpnHandler::OnJavascriptAllowed() {}

void BraveVpnHandler::OnJavascriptDisallowed() {}
