/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/ipfs_ui.h"

#include <optional>
#include <utility>

#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/ipfs/addresses_config.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/repo_stats.h"
#include "brave/components/ipfs_ui/resources/grit/ipfs_generated_map.h"
#include "chrome/browser/browser_process_impl.h"
#include "components/component_updater/component_updater_service.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/update_client/crx_update_item.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

std::string GetIPFSUpdaterVersion() {
  // component_updater::ComponentUpdateService* cus =
  //     g_browser_process->component_updater();
  // if (!cus)
  //   return std::string();

  // for (const auto& component : cus->GetComponents()) {
  //   if (component.id != ipfs::kIpfsClientComponentId)
  //     continue;
  //   return component.version.GetString();
  // }
  return std::string();
}

void CallOnGetDaemonStatus(content::WebUI* web_ui, const std::string& error) {
  ipfs::IpfsService* service = ipfs::IpfsServiceFactory::GetForContext(
      web_ui->GetWebContents()->GetBrowserContext());
  if (!service) {
    return;
  }

  base::Value::Dict value;
  value.Set("installed", service->IsIPFSExecutableAvailable());
  value.Set("launched", service->IsDaemonLaunched());
  value.Set("error", error);
  web_ui->CallJavascriptFunctionUnsafe("ipfs.onGetDaemonStatus", value);
}

}  // namespace

IPFSDOMHandler::IPFSDOMHandler() : weak_ptr_factory_{this} {}

IPFSDOMHandler::~IPFSDOMHandler() = default;

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
  // auto* source = CreateAndAddWebUIDataSource(web_ui, name, kIpfsGenerated,
  //                                            kIpfsGeneratedSize, IDR_IPFS_HTML);
  // source->AddString("componentVersion", ipfs::kIpfsClientComponentName);
  web_ui->AddMessageHandler(std::make_unique<IPFSDOMHandler>());
}

IPFSUI::~IPFSUI() = default;

void IPFSDOMHandler::HandleGetConnectedPeers(const base::Value::List& args) {
  DCHECK_EQ(args.size(), 0U);
  if (!web_ui()->CanCallJavascript())
    return;

  ipfs::IpfsService* service = ipfs::IpfsServiceFactory::GetForContext(
      web_ui()->GetWebContents()->GetBrowserContext());
  if (!service) {
    return;
  }
  service->GetConnectedPeers(
      base::BindOnce(&IPFSDOMHandler::OnGetConnectedPeers,
                     weak_ptr_factory_.GetWeakPtr()),
      std::nullopt);
}

void IPFSDOMHandler::OnGetConnectedPeers(
    bool success,
    const std::vector<std::string>& peers) {
  if (!web_ui()->CanCallJavascript())
    return;
  base::Value::Dict stats_value;
  stats_value.Set("peerCount", static_cast<double>(peers.size()));
  web_ui()->CallJavascriptFunctionUnsafe("ipfs.onGetConnectedPeers",
                                         stats_value);
}

void IPFSDOMHandler::HandleGetAddressesConfig(const base::Value::List& args) {
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

  base::Value::Dict config_value;
  config_value.Set("api", config.api);
  config_value.Set("gateway", config.gateway);
  base::Value::List swarm_value;
  for (const auto& addr : config.swarm)
    swarm_value.Append(addr);
  config_value.Set("swarm", std::move(swarm_value));

  web_ui()->CallJavascriptFunctionUnsafe("ipfs.onGetAddressesConfig",
                                         config_value);
}

void IPFSDOMHandler::HandleGetDaemonStatus(const base::Value::List& args) {
  DCHECK_EQ(args.size(), 0U);
  if (!web_ui()->CanCallJavascript())
    return;

  CallOnGetDaemonStatus(web_ui(), std::string());
}

void IPFSDOMHandler::HandleLaunchDaemon(const base::Value::List& args) {
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
}

void IPFSDOMHandler::OnIpfsShutdown() {
  CallOnGetDaemonStatus(web_ui(), std::string());
}

void IPFSDOMHandler::HandleShutdownDaemon(const base::Value::List& args) {
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

void IPFSDOMHandler::HandleRestartDaemon(const base::Value::List& args) {
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

void IPFSDOMHandler::HandleGetRepoStats(const base::Value::List& args) {
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

  base::Value::Dict stats_value;
  stats_value.Set("objects", static_cast<double>(stats.objects));
  stats_value.Set("size", static_cast<double>(stats.size));
  stats_value.Set("storage", static_cast<double>(stats.storage_max));
  stats_value.Set("path", stats.path);
  stats_value.Set("version", stats.version);

  web_ui()->CallJavascriptFunctionUnsafe("ipfs.onGetRepoStats", stats_value);
}

void IPFSDOMHandler::HandleGetNodeInfo(const base::Value::List& args) {
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

void IPFSDOMHandler::HandleGarbageCollection(const base::Value::List& args) {
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

  base::Value::Dict result;
  result.Set("error", error);
  result.Set("success", success);
  result.Set("started", false);
  web_ui()->CallJavascriptFunctionUnsafe("ipfs.onGarbageCollection", result);
}
std::string IPFSDOMHandler::GetIpfsClientUpdaterVersion() const {
  if (client_updater_version_for_testing_)
    return client_updater_version_for_testing_.value();
  return GetIPFSUpdaterVersion();
}

void IPFSDOMHandler::OnGetNodeInfo(bool success, const ipfs::NodeInfo& info) {
  if (!web_ui()->CanCallJavascript())
    return;

  base::Value::Dict node_value;
  node_value.Set("id", info.id);
  node_value.Set("version", info.version);
  auto extension_version = GetIpfsClientUpdaterVersion();
  if (!extension_version.empty()) {
    node_value.Set("component_version", extension_version);
  }
  web_ui()->CallJavascriptFunctionUnsafe("ipfs.onGetNodeInfo", node_value);
}
