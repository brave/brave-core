/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/ipfs_ui.h"

#include <utility>

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/ipfs/ipfs_service.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/ipfs/browser/addresses_config.h"
#include "brave/components/ipfs_ui/resources/grit/ipfs_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_ui_data_source.h"

IPFSDOMHandler::IPFSDOMHandler() : weak_ptr_factory_{this} {}

IPFSDOMHandler::~IPFSDOMHandler() {}

void IPFSDOMHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "ipfs.getConnectedPeers",
      base::BindRepeating(&IPFSDOMHandler::HandleGetConnectedPeers,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "ipfs.getAddressesConfig",
      base::BindRepeating(&IPFSDOMHandler::HandleGetAddressesConfig,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "ipfs.getDaemonStatus",
      base::BindRepeating(&IPFSDOMHandler::HandleGetDaemonStatus,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "ipfs.launchDaemon",
      base::BindRepeating(&IPFSDOMHandler::HandleLaunchDaemon,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "ipfs.shutdownDaemon",
      base::BindRepeating(&IPFSDOMHandler::HandleShutdownDaemon,
                          base::Unretained(this)));
}

IPFSUI::IPFSUI(content::WebUI* web_ui, const std::string& name)
    : BasicUI(web_ui, name, kIpfsGenerated,
        kIpfsGeneratedSize, IDR_IPFS_HTML) {
  web_ui->AddMessageHandler(std::make_unique<IPFSDOMHandler>());
}

IPFSUI::~IPFSUI() {
}

void IPFSDOMHandler::HandleGetConnectedPeers(const base::ListValue* args) {
  DCHECK_EQ(args->GetSize(), 0U);
  if (!web_ui()->CanCallJavascript())
    return;

  ipfs::IpfsService* service =
      ipfs::IpfsServiceFactory::GetForContext(Profile::FromWebUI(web_ui()));
  if (!service) {
    return;
  }

  service->GetConnectedPeers(
      base::BindOnce(&IPFSDOMHandler::OnGetConnectedPeers,
                     weak_ptr_factory_.GetWeakPtr()));
}

void IPFSDOMHandler::OnGetConnectedPeers(bool success,
    const std::vector<std::string>& peers) {
  if (!web_ui()->CanCallJavascript())
    return;

  web_ui()->CallJavascriptFunctionUnsafe("ipfs.onGetConnectedPeers",
      base::Value(static_cast<int>(peers.size())));
}

void IPFSDOMHandler::HandleGetAddressesConfig(const base::ListValue* args) {
  DCHECK_EQ(args->GetSize(), 0U);
  if (!web_ui()->CanCallJavascript())
    return;

  ipfs::IpfsService* service =
      ipfs::IpfsServiceFactory::GetForContext(Profile::FromWebUI(web_ui()));
  if (!service) {
    return;
  }

  service->GetAddressesConfig(
      base::BindOnce(&IPFSDOMHandler::OnGetAddressesConfig,
                     weak_ptr_factory_.GetWeakPtr()));
}

void IPFSDOMHandler::OnGetAddressesConfig(bool success,
    const ipfs::AddressesConfig& config) {
  if (!web_ui()->CanCallJavascript())
    return;

  base::Value config_value(base::Value::Type::DICTIONARY);
  config_value.SetStringKey("api", config.api);
  config_value.SetStringKey("gateway", config.gateway);
  base::Value swarm_value(base::Value::Type::LIST);
  for (const auto& addr : config.swarm)
    swarm_value.Append(addr);
  config_value.SetKey("swarm", std::move(swarm_value));

  web_ui()->CallJavascriptFunctionUnsafe("ipfs.onGetAddressesConfig",
                                         std::move(config_value));
}

void IPFSDOMHandler::HandleGetDaemonStatus(const base::ListValue* args) {
  DCHECK_EQ(args->GetSize(), 0U);
  if (!web_ui()->CanCallJavascript())
    return;

  ipfs::IpfsService* service =
      ipfs::IpfsServiceFactory::GetForContext(Profile::FromWebUI(web_ui()));
  if (!service) {
    return;
  }

  base::Value value(base::Value::Type::DICTIONARY);
  value.SetBoolKey("launched", service->IsDaemonLaunched());
  web_ui()->CallJavascriptFunctionUnsafe("ipfs.onGetDaemonStatus",
                                         std::move(value));
}

void IPFSDOMHandler::HandleLaunchDaemon(const base::ListValue* args) {
  DCHECK_EQ(args->GetSize(), 0U);
  if (!web_ui()->CanCallJavascript())
    return;

  ipfs::IpfsService* service =
      ipfs::IpfsServiceFactory::GetForContext(Profile::FromWebUI(web_ui()));
  if (!service) {
    return;
  }

  service->LaunchDaemon(
      base::BindOnce(&IPFSDOMHandler::OnLaunchDaemon,
                     weak_ptr_factory_.GetWeakPtr()));
}

void IPFSDOMHandler::OnLaunchDaemon(bool success) {
  if (!web_ui()->CanCallJavascript() || !success)
    return;

  base::Value value(base::Value::Type::DICTIONARY);
  value.SetBoolKey("launched", true);
  web_ui()->CallJavascriptFunctionUnsafe("ipfs.onGetDaemonStatus",
                                         std::move(value));
}

void IPFSDOMHandler::HandleShutdownDaemon(const base::ListValue* args) {
  DCHECK_EQ(args->GetSize(), 0U);
  if (!web_ui()->CanCallJavascript())
    return;

  ipfs::IpfsService* service =
      ipfs::IpfsServiceFactory::GetForContext(Profile::FromWebUI(web_ui()));
  if (!service) {
    return;
  }

  service->ShutdownDaemon(
      base::BindOnce(&IPFSDOMHandler::OnShutdownDaemon,
                     weak_ptr_factory_.GetWeakPtr()));
}

void IPFSDOMHandler::OnShutdownDaemon(bool success) {
  if (!web_ui()->CanCallJavascript() || !success)
    return;

  base::Value value(base::Value::Type::DICTIONARY);
  value.SetBoolKey("launched", false);
  web_ui()->CallJavascriptFunctionUnsafe("ipfs.onGetDaemonStatus",
                                         std::move(value));
}
