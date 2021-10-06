/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/ipfs_ui.h"

#include <utility>

#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/ipfs/addresses_config.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/repo_stats.h"
#include "brave/components/ipfs_ui/resources/grit/ipfs_generated_map.h"
#include "chrome/browser/browser_process_impl.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/update_client/crx_update_item.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

void CallOnGetDaemonStatus(content::WebUI* web_ui, const std::string& error) {
  ipfs::IpfsService* service = ipfs::IpfsServiceFactory::GetForContext(
      web_ui->GetWebContents()->GetBrowserContext());
  if (!service) {
    return;
  }

  base::Value value(base::Value::Type::DICTIONARY);
  value.SetBoolKey("installed", service->IsIPFSExecutableAvailable());
  value.SetBoolKey("launched", service->IsDaemonLaunched());
  value.SetStringKey("error", error);
  web_ui->CallJavascriptFunctionUnsafe("ipfs.onGetDaemonStatus",
                                       std::move(value));
}

}  // namespace

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
  web_ui()->RegisterMessageCallback(
      "ipfs.restartDaemon",
      base::BindRepeating(&IPFSDOMHandler::HandleRestartDaemon,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "ipfs.getRepoStats",
      base::BindRepeating(&IPFSDOMHandler::HandleGetRepoStats,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "ipfs.getNodeInfo",
      base::BindRepeating(&IPFSDOMHandler::HandleGetNodeInfo,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "ipfs.garbageCollection",
      base::BindRepeating(&IPFSDOMHandler::HandleGarbageCollection,
                          base::Unretained(this)));
  ipfs::IpfsService* service = ipfs::IpfsServiceFactory::GetForContext(
      web_ui()->GetWebContents()->GetBrowserContext());
  if (!service) {
    return;
  }

  service_observer_.Observe(service);
}

IPFSUI::IPFSUI(content::WebUI* web_ui, const std::string& name)
    : WebUIController(web_ui) {
  CreateAndAddWebUIDataSource(web_ui, name, kIpfsGenerated, kIpfsGeneratedSize,
                              IDR_IPFS_HTML);
  web_ui->AddMessageHandler(std::make_unique<IPFSDOMHandler>());
}

IPFSUI::~IPFSUI() {}

void IPFSDOMHandler::HandleGetConnectedPeers(base::Value::ConstListView args) {
  DCHECK_EQ(args.size(), 0U);
  if (!web_ui()->CanCallJavascript())
    return;

  ipfs::IpfsService* service = ipfs::IpfsServiceFactory::GetForContext(
      web_ui()->GetWebContents()->GetBrowserContext());
  if (!service) {
    return;
  }
  service->GetConnectedPeers(base::BindOnce(
      &IPFSDOMHandler::OnGetConnectedPeers, weak_ptr_factory_.GetWeakPtr()));
}

void IPFSDOMHandler::OnGetConnectedPeers(
    bool success,
    const std::vector<std::string>& peers) {
  if (!web_ui()->CanCallJavascript())
    return;
  base::Value stats_value(base::Value::Type::DICTIONARY);
  stats_value.SetDoubleKey("peerCount", peers.size());
  web_ui()->CallJavascriptFunctionUnsafe("ipfs.onGetConnectedPeers",
                                         std::move(stats_value));
}

void IPFSDOMHandler::HandleGetAddressesConfig(base::Value::ConstListView args) {
  DCHECK_EQ(args.size(), 0U);
  if (!web_ui()->CanCallJavascript())
    return;

  ipfs::IpfsService* service = ipfs::IpfsServiceFactory::GetForContext(
      web_ui()->GetWebContents()->GetBrowserContext());
  if (!service) {
    return;
  }

  service->GetAddressesConfig(base::BindOnce(
      &IPFSDOMHandler::OnGetAddressesConfig, weak_ptr_factory_.GetWeakPtr()));
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

void IPFSDOMHandler::HandleGetDaemonStatus(base::Value::ConstListView args) {
  DCHECK_EQ(args.size(), 0U);
  if (!web_ui()->CanCallJavascript())
    return;

  CallOnGetDaemonStatus(web_ui(), std::string());
}

void IPFSDOMHandler::HandleLaunchDaemon(base::Value::ConstListView args) {
  DCHECK_EQ(args.size(), 0U);
  if (!web_ui()->CanCallJavascript())
    return;
  LaunchDaemon();
}

void IPFSDOMHandler::LaunchDaemon() {
  ipfs::IpfsService* service = ipfs::IpfsServiceFactory::GetForContext(
      web_ui()->GetWebContents()->GetBrowserContext());
  if (!service) {
    return;
  }
  service->LaunchDaemon(base::NullCallback());
}

void IPFSDOMHandler::OnIpfsLaunched(bool success, int64_t pid) {
  if (!web_ui()->CanCallJavascript())
    return;
  std::string error;
  if (!success) {
    error = l10n_util::GetStringUTF8(IDS_IPFS_NODE_LAUNCH_ERROR);
  }

  CallOnGetDaemonStatus(web_ui(), error);
}

void IPFSDOMHandler::OnInstallationEvent(ipfs::ComponentUpdaterEvents event) {
  if (event == ipfs::ComponentUpdaterEvents::COMPONENT_UPDATE_DOWNLOADING) {
    if (!g_browser_process->component_updater())
      return;
    auto* updater = g_browser_process->component_updater();
    update_client::CrxUpdateItem item;
    if (updater->GetComponentDetails(ipfs::kIpfsClientComponentId, &item)) {
      if (item.downloaded_bytes > 0 && item.total_bytes > 0) {
        base::Value value(base::Value::Type::DICTIONARY);
        value.SetDoubleKey("total_bytes", item.total_bytes);
        value.SetDoubleKey("downloaded_bytes", item.downloaded_bytes);
        web_ui()->CallJavascriptFunctionUnsafe("ipfs.onInstallationProgress",
                                               std::move(value));
      }
    }
  } else if (event == ipfs::ComponentUpdaterEvents::COMPONENT_UPDATE_ERROR) {
    base::Value value(base::Value::Type::DICTIONARY);
    value.SetBoolKey("installed", false);
    value.SetStringKey(
        "error", l10n_util::GetStringUTF8(IDS_IPFS_NODE_INSTALLATION_ERROR));
    web_ui()->CallJavascriptFunctionUnsafe("ipfs.onGetDaemonStatus",
                                           std::move(value));
  }
}

void IPFSDOMHandler::OnIpfsShutdown() {
  CallOnGetDaemonStatus(web_ui(), std::string());
}

void IPFSDOMHandler::HandleShutdownDaemon(base::Value::ConstListView args) {
  DCHECK_EQ(args.size(), 0U);
  if (!web_ui()->CanCallJavascript())
    return;

  ipfs::IpfsService* service = ipfs::IpfsServiceFactory::GetForContext(
      web_ui()->GetWebContents()->GetBrowserContext());
  if (!service) {
    return;
  }

  service->ShutdownDaemon(base::NullCallback());
}

void IPFSDOMHandler::HandleRestartDaemon(base::Value::ConstListView args) {
  DCHECK_EQ(args.size(), 0U);
  if (!web_ui()->CanCallJavascript())
    return;

  ipfs::IpfsService* service = ipfs::IpfsServiceFactory::GetForContext(
      web_ui()->GetWebContents()->GetBrowserContext());
  if (!service) {
    return;
  }
  if (service->IsDaemonLaunched()) {
    service->RestartDaemon();
  }
}

void IPFSDOMHandler::HandleGetRepoStats(base::Value::ConstListView args) {
  DCHECK_EQ(args.size(), 0U);
  if (!web_ui()->CanCallJavascript())
    return;

  ipfs::IpfsService* service = ipfs::IpfsServiceFactory::GetForContext(
      web_ui()->GetWebContents()->GetBrowserContext());
  if (!service) {
    return;
  }

  service->GetRepoStats(base::BindOnce(&IPFSDOMHandler::OnGetRepoStats,
                                       weak_ptr_factory_.GetWeakPtr()));
}

void IPFSDOMHandler::OnGetRepoStats(bool success,
                                    const ipfs::RepoStats& stats) {
  if (!web_ui()->CanCallJavascript())
    return;

  base::Value stats_value(base::Value::Type::DICTIONARY);
  stats_value.SetDoubleKey("objects", stats.objects);
  stats_value.SetDoubleKey("size", stats.size);
  stats_value.SetDoubleKey("storage", stats.storage_max);
  stats_value.SetStringKey("path", stats.path);
  stats_value.SetStringKey("version", stats.version);

  web_ui()->CallJavascriptFunctionUnsafe("ipfs.onGetRepoStats",
                                         std::move(stats_value));
}

void IPFSDOMHandler::HandleGetNodeInfo(base::Value::ConstListView args) {
  DCHECK_EQ(args.size(), 0U);
  if (!web_ui()->CanCallJavascript())
    return;

  ipfs::IpfsService* service = ipfs::IpfsServiceFactory::GetForContext(
      web_ui()->GetWebContents()->GetBrowserContext());
  if (!service) {
    return;
  }

  service->GetNodeInfo(base::BindOnce(&IPFSDOMHandler::OnGetNodeInfo,
                                      weak_ptr_factory_.GetWeakPtr()));
}

void IPFSDOMHandler::HandleGarbageCollection(base::Value::ConstListView args) {
  DCHECK_EQ(args.size(), 0U);
  if (!web_ui()->CanCallJavascript())
    return;

  ipfs::IpfsService* service = ipfs::IpfsServiceFactory::GetForContext(
      web_ui()->GetWebContents()->GetBrowserContext());
  if (!service) {
    return;
  }

  service->RunGarbageCollection(base::BindOnce(
      &IPFSDOMHandler::OnGarbageCollection, weak_ptr_factory_.GetWeakPtr()));
}

void IPFSDOMHandler::OnGarbageCollection(bool success,
                                         const std::string& error) {
  if (!web_ui()->CanCallJavascript())
    return;

  base::Value result(base::Value::Type::DICTIONARY);
  result.SetStringKey("error", error);
  result.SetBoolKey("success", success);
  result.SetBoolKey("started", false);
  web_ui()->CallJavascriptFunctionUnsafe("ipfs.onGarbageCollection",
                                         std::move(result));
}

void IPFSDOMHandler::OnGetNodeInfo(bool success, const ipfs::NodeInfo& info) {
  if (!web_ui()->CanCallJavascript())
    return;

  base::Value node_value(base::Value::Type::DICTIONARY);
  node_value.SetStringKey("id", info.id);
  node_value.SetStringKey("version", info.version);

  web_ui()->CallJavascriptFunctionUnsafe("ipfs.onGetNodeInfo",
                                         std::move(node_value));
}
