/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_service.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/path_service.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/components/ipfs/browser/ipfs_json_parser.h"
#include "brave/components/ipfs/common/ipfs_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/service_sandbox_type.h"
#include "chrome/common/chrome_paths.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/service_process_host.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "net/http/http_request_headers.h"

namespace {

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("ipfs_service", R"(
      semantics {
        sender: "IPFS service"
        description:
          "This service is used to communicate with IPFS daemon "
          "on behalf of the user interacting with the actions in brvae://ipfs."
        trigger:
          "Triggered by actions in brave://ipfs."
        data:
          "Options of the commands."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting:
          "You can enable or disable this feature in brave://settings."
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

}  // namespace

namespace ipfs {

IpfsService::IpfsService(content::BrowserContext* context) {
  url_loader_factory_ =
      content::BrowserContext::GetDefaultStoragePartition(context)
          ->GetURLLoaderFactoryForBrowserProcess();

  g_brave_browser_process->ipfs_client_updater()->AddObserver(this);
  OnExecutableReady(GetIpfsExecutablePath());
}

IpfsService::~IpfsService() = default;

base::FilePath IpfsService::GetIpfsExecutablePath() {
  return g_brave_browser_process->ipfs_client_updater()->GetExecutablePath();
}

void IpfsService::OnExecutableReady(const base::FilePath& path) {
  if (path.empty())
    return;

  g_brave_browser_process->ipfs_client_updater()->RemoveObserver(this);
  LaunchIfNotRunning(path);
}

void IpfsService::LaunchIfNotRunning(const base::FilePath& executable_path) {
  if (ipfs_service_.is_bound())
    return;

  content::ServiceProcessHost::Launch(
      ipfs_service_.BindNewPipeAndPassReceiver(),
      content::ServiceProcessHost::Options()
        .WithDisplayName(IDS_UTILITY_PROCESS_IPFS_NAME)
        .Pass());

  ipfs_service_.set_disconnect_handler(
      base::BindOnce(
        &IpfsService::OnIpfsCrashed,
        base::Unretained(this)));

  ipfs_service_->SetCrashHandler(
      base::Bind(&IpfsService::OnIpfsDaemonCrashed,
        base::Unretained(this)));

  base::FilePath user_data_dir;
  base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
  DCHECK(!user_data_dir.empty());

  base::FilePath data_root_path =
    user_data_dir.Append(FILE_PATH_LITERAL("ipfs"));
  base::FilePath config_path =
    data_root_path.Append(FILE_PATH_LITERAL("config"));

  auto config = mojom::IpfsConfig::New(
      executable_path,
      config_path,
      data_root_path);

  ipfs_service_->Launch(
      std::move(config),
      base::Bind(&IpfsService::OnIpfsLaunched,
        base::Unretained(this)));
}

void IpfsService::OnIpfsCrashed() {
  VLOG(0) << "IPFS utility process crashed";
  Shutdown();
}

void IpfsService::OnIpfsDaemonCrashed(int64_t pid) {
  VLOG(0) << "IPFS daemon crashed";
  Shutdown();
}

void IpfsService::OnIpfsLaunched(bool result, int64_t pid) {
  if (result) {
    ipfs_pid_ = pid;
  } else {
    VLOG(0) << "Failed to launch IPFS";
    Shutdown();
  }
}

void IpfsService::Shutdown() {
  if (ipfs_service_.is_bound()) {
    ipfs_service_->Shutdown();
  }

  ipfs_service_.reset();
  ipfs_pid_ = -1;
}

void IpfsService::GetConnectedPeers(GetConnectedPeersCallback callback) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(kSwarmPeersAPIURL);
  request->method = "POST";
  request->headers.SetHeader(net::HttpRequestHeaders::kOrigin,
                             kHttpAPIServerEndpoint);

  url_loader_ = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  url_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&IpfsService::OnGetConnectedPeers,
                     base::Unretained(this),
                     std::move(callback)));
}

void IpfsService::OnGetConnectedPeers(
    GetConnectedPeersCallback callback,
    std::unique_ptr<std::string> response_body) {
  int error_code = url_loader_->NetError();
  url_loader_.reset();

  if (error_code != net::OK) {
    std::move(callback).Run(false, std::vector<std::string>{});
    return;
  }

  std::vector<std::string> peers;
  bool success = IPFSJSONParser::GetPeersFromJSON(*response_body, &peers);
  std::move(callback).Run(success, peers);
}

}  // namespace ipfs
