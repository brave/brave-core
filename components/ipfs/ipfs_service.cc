/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_service.h"

#include <utility>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_json_parser.h"
#include "brave/components/ipfs/ipfs_ports.h"
#include "brave/components/ipfs/ipfs_service_observer.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/pref_names.h"
#include "brave/components/ipfs/service_sandbox_type.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/service_process_host.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/url_util.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"

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

std::pair<bool, std::string> LoadConfigFileOnFileTaskRunner(
    const base::FilePath& path) {
  std::string data;
  bool success = base::ReadFileToString(path, &data);
  std::pair<bool, std::string> result;
  result.first = success;
  if (success) {
    result.second = data;
  }
  return result;
}

}  // namespace

namespace ipfs {

IpfsService::IpfsService(content::BrowserContext* context,
                         ipfs::BraveIpfsClientUpdater* ipfs_client_updater,
                         const base::FilePath& user_data_dir,
                         version_info::Channel channel)
    : context_(context),
      server_endpoint_(GetAPIServer(channel)),
      user_data_dir_(user_data_dir),
      ipfs_client_updater_(ipfs_client_updater),
      channel_(channel),
      file_task_runner_(base::CreateSequencedTaskRunner(
          {base::ThreadPool(), base::MayBlock(),
           base::TaskPriority::BEST_EFFORT,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
      ipfs_p3a(this, context),
      weak_factory_(this) {
  DCHECK(!user_data_dir.empty());
  url_loader_factory_ =
      content::BrowserContext::GetDefaultStoragePartition(context)
          ->GetURLLoaderFactoryForBrowserProcess();

  // Return early since g_brave_browser_process and ipfs_client_updater are not
  // available in unit tests.
  if (ipfs_client_updater_) {
    ipfs_client_updater_->AddObserver(this);
    OnExecutableReady(ipfs_client_updater_->GetExecutablePath());
  }
}

IpfsService::~IpfsService() = default;

// static
void IpfsService::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kIPFSEnabled, true);
  registry->RegisterIntegerPref(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_ASK));
  registry->RegisterBooleanPref(kIPFSAutoFallbackToGateway, false);
  registry->RegisterBooleanPref(kIPFSAutoRedirectGateway, false);
  registry->RegisterBooleanPref(kIPFSAutoRedirectDNSLink, false);
  registry->RegisterIntegerPref(kIPFSInfobarCount, 0);
  registry->RegisterStringPref(kIPFSPublicGatewayAddress, kDefaultIPFSGateway);
  registry->RegisterFilePathPref(kIPFSBinaryPath, base::FilePath());
}

base::FilePath IpfsService::GetIpfsExecutablePath() const {
  PrefService* prefs = user_prefs::UserPrefs::Get(context_);
  return prefs->GetFilePath(kIPFSBinaryPath);
}

void IpfsService::OnInstallationEvent(ComponentUpdaterEvents event) {
  for (auto& observer : observers_) {
    observer.OnInstallationEvent(event);
  }
}

void IpfsService::OnExecutableReady(const base::FilePath& path) {
  if (path.empty())
    return;

  PrefService* prefs = user_prefs::UserPrefs::Get(context_);
  prefs->SetFilePath(kIPFSBinaryPath, path);

  if (ipfs_client_updater_) {
    ipfs_client_updater_->RemoveObserver(this);
  }
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
      base::BindOnce(&IpfsService::OnIpfsCrashed, base::Unretained(this)));
  ipfs_service_->SetCrashHandler(
      base::Bind(&IpfsService::OnIpfsDaemonCrashed, base::Unretained(this)));

  auto config = mojom::IpfsConfig::New(
      executable_path, GetConfigFilePath(), GetDataPath(),
      GetGatewayPort(channel_), GetAPIPort(channel_), GetSwarmPort(channel_));

  ipfs_service_->Launch(
      std::move(config),
      base::Bind(&IpfsService::OnIpfsLaunched, base::Unretained(this)));
}

void IpfsService::OnIpfsCrashed() {
  VLOG(0) << "IPFS utility process crashed";
  Shutdown();
}

void IpfsService::OnIpfsDaemonCrashed(int64_t pid) {
  VLOG(0) << "IPFS daemon crashed";
  Shutdown();
}

base::FilePath IpfsService::GetDataPath() const {
  return user_data_dir_.Append(FILE_PATH_LITERAL("brave_ipfs"));
}

base::FilePath IpfsService::GetConfigFilePath() const {
  base::FilePath config_path =
      GetDataPath().Append(FILE_PATH_LITERAL("config"));
  return config_path;
}

void IpfsService::NotifyDaemonLaunchCallbacks(bool result) {
  while (!pending_launch_callbacks_.empty()) {
    if (pending_launch_callbacks_.front())
      std::move(pending_launch_callbacks_.front()).Run(result);
    pending_launch_callbacks_.pop();
  }
}

void IpfsService::OnIpfsLaunched(bool result, int64_t pid) {
  if (result) {
    ipfs_pid_ = pid;
  } else {
    VLOG(0) << "Failed to launch IPFS";
    Shutdown();
  }

  NotifyDaemonLaunchCallbacks(result && pid > 0);

  for (auto& observer : observers_) {
    observer.OnIpfsLaunched(result, pid);
  }
}

void IpfsService::Shutdown() {
  if (ipfs_service_.is_bound()) {
    ipfs_service_->Shutdown();
  }

  ipfs_service_.reset();
  ipfs_pid_ = -1;
}

std::unique_ptr<network::SimpleURLLoader> IpfsService::CreateURLLoader(
    const GURL& gurl) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = gurl;
  request->method = "POST";

  // Remove trailing "/".
  std::string origin = server_endpoint_.spec();
  if (base::EndsWith(origin, "/", base::CompareCase::INSENSITIVE_ASCII)) {
    origin.pop_back();
  }
  request->headers.SetHeader(net::HttpRequestHeaders::kOrigin, origin);

  auto url_loader = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  return url_loader;
}

void IpfsService::GetConnectedPeers(GetConnectedPeersCallback callback) {
  if (!IsDaemonLaunched()) {
    if (callback)
      std::move(callback).Run(false, std::vector<std::string>{});
    return;
  }

  if (skip_get_connected_peers_callback_for_test_) {
    // Early return for tests that wish to  manually run the callback with
    // desired values directly, could be useful in unit tests.
    connected_peers_function_called_ = true;
    return;
  }

  auto url_loader = CreateURLLoader(server_endpoint_.Resolve(kSwarmPeersPath));
  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));

  iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&IpfsService::OnGetConnectedPeers, base::Unretained(this),
                     std::move(iter), std::move(callback)));
}

void IpfsService::OnGetConnectedPeers(
    SimpleURLLoaderList::iterator iter,
    GetConnectedPeersCallback callback,
    std::unique_ptr<std::string> response_body) {
  auto* url_loader = iter->get();
  int error_code = url_loader->NetError();
  int response_code = -1;
  if (url_loader->ResponseInfo() && url_loader->ResponseInfo()->headers)
    response_code = url_loader->ResponseInfo()->headers->response_code();
  url_loaders_.erase(iter);

  std::vector<std::string> peers;
  bool success = (error_code == net::OK && response_code == net::HTTP_OK);
  if (!success) {
    VLOG(1) << "Fail to get connected peers, error_code = " << error_code
            << " response_code = " << response_code;
  }

  if (success)
    success = IPFSJSONParser::GetPeersFromJSON(*response_body, &peers);

  if (callback)
    std::move(callback).Run(success, peers);

  for (auto& observer : observers_) {
    observer.OnGetConnectedPeers(success, peers);
  }
}

void IpfsService::GetAddressesConfig(GetAddressesConfigCallback callback) {
  if (!IsDaemonLaunched()) {
    std::move(callback).Run(false, AddressesConfig());
    return;
  }

  GURL gurl = net::AppendQueryParameter(server_endpoint_.Resolve(kConfigPath),
                                        kArgQueryParam, kAddressesField);
  auto url_loader = CreateURLLoader(gurl);
  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));

  iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&IpfsService::OnGetAddressesConfig, base::Unretained(this),
                     std::move(iter), std::move(callback)));
}

void IpfsService::OnGetAddressesConfig(
    SimpleURLLoaderList::iterator iter,
    GetAddressesConfigCallback callback,
    std::unique_ptr<std::string> response_body) {
  auto* url_loader = iter->get();
  int error_code = url_loader->NetError();
  int response_code = -1;
  if (url_loader->ResponseInfo() && url_loader->ResponseInfo()->headers)
    response_code = url_loader->ResponseInfo()->headers->response_code();
  url_loaders_.erase(iter);

  ipfs::AddressesConfig addresses_config;
  if (error_code != net::OK || response_code != net::HTTP_OK) {
    VLOG(1) << "Fail to get addresses config, error_code = " << error_code
            << " response_code = " << response_code;
    std::move(callback).Run(false, addresses_config);
    return;
  }

  bool success = IPFSJSONParser::GetAddressesConfigFromJSON(*response_body,
                                                            &addresses_config);
  std::move(callback).Run(success, addresses_config);
}

bool IpfsService::IsDaemonLaunched() const {
  if (allow_ipfs_launch_for_test_) {
    return true;
  }
  return ipfs_pid_ > 0;
}

void IpfsService::LaunchDaemon(LaunchDaemonCallback callback) {
  if (IsDaemonLaunched()) {
    if (callback)
      std::move(callback).Run(true);
    return;
  }

  // Wait if previous launch request in progress.
  if (!pending_launch_callbacks_.empty()) {
    if (callback)
      pending_launch_callbacks_.push(std::move(callback));
    return;
  }
  if (callback)
    pending_launch_callbacks_.push(std::move(callback));
  base::FilePath path(GetIpfsExecutablePath());
  if (path.empty()) {
    // Daemon will be launched later in OnExecutableReady.
    RegisterIpfsClientUpdater();
  } else {
    LaunchIfNotRunning(path);
  }
}

void IpfsService::ShutdownDaemon(ShutdownDaemonCallback callback) {
  if (IsDaemonLaunched()) {
    Shutdown();
  }

  for (auto& observer : observers_) {
    observer.OnIpfsShutdown();
  }

  if (callback)
    std::move(callback).Run(true);
}

void IpfsService::GetConfig(GetConfigCallback callback) {
  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&LoadConfigFileOnFileTaskRunner, GetConfigFilePath()),
      base::BindOnce(&IpfsService::OnConfigLoaded, weak_factory_.GetWeakPtr(),
                     std::move(callback)));
}

void IpfsService::OnConfigLoaded(GetConfigCallback callback,
                                 const std::pair<bool, std::string>& result) {
  std::move(callback).Run(result.first, result.second);
}

bool IpfsService::IsIPFSExecutableAvailable() const {
  return !GetIpfsExecutablePath().empty();
}

void IpfsService::AddObserver(IpfsServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void IpfsService::RemoveObserver(IpfsServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

void IpfsService::RegisterIpfsClientUpdater() {
  if (ipfs_client_updater_) {
    ipfs_client_updater_->Register();
  }
}

void IpfsService::SetAllowIpfsLaunchForTest(bool launched) {
  allow_ipfs_launch_for_test_ = launched;
}

void IpfsService::SetServerEndpointForTest(const GURL& gurl) {
  server_endpoint_ = gurl;
}

void IpfsService::RunLaunchDaemonCallbackForTest(bool result) {
  NotifyDaemonLaunchCallbacks(result);
}

void IpfsService::SetSkipGetConnectedPeersCallbackForTest(bool skip) {
  skip_get_connected_peers_callback_for_test_ = skip;
}

void IpfsService::SetGetConnectedPeersCalledForTest(bool value) {
  connected_peers_function_called_ = value;
}

bool IpfsService::WasConnectedPeersCalledForTest() const {
  return connected_peers_function_called_;
}

IPFSResolveMethodTypes IpfsService::GetIPFSResolveMethodType() const {
  PrefService* prefs = user_prefs::UserPrefs::Get(context_);
  return static_cast<IPFSResolveMethodTypes>(
      prefs->GetInteger(kIPFSResolveMethod));
}

void IpfsService::GetRepoStats(GetRepoStatsCallback callback) {
  if (!IsDaemonLaunched()) {
    std::move(callback).Run(false, RepoStats());
    return;
  }

  GURL gurl =
      net::AppendQueryParameter(server_endpoint_.Resolve(ipfs::kRepoStatsPath),
                                ipfs::kRepoStatsHumanReadableParamName,
                                ipfs::kRepoStatsHumanReadableParamValue);
  auto url_loader = CreateURLLoader(gurl);
  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));

  iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&IpfsService::OnRepoStats, base::Unretained(this),
                     std::move(iter), std::move(callback)));
}

void IpfsService::OnRepoStats(SimpleURLLoaderList::iterator iter,
                              GetRepoStatsCallback callback,
                              std::unique_ptr<std::string> response_body) {
  auto* url_loader = iter->get();
  int error_code = url_loader->NetError();
  int response_code = -1;
  if (url_loader->ResponseInfo() && url_loader->ResponseInfo()->headers)
    response_code = url_loader->ResponseInfo()->headers->response_code();
  url_loaders_.erase(iter);

  ipfs::RepoStats repo_stats;
  if (error_code != net::OK || response_code != net::HTTP_OK) {
    VLOG(1) << "Fail to get repro stats, error_code = " << error_code
            << " response_code = " << response_code;
    std::move(callback).Run(false, repo_stats);
    return;
  }

  bool success =
      IPFSJSONParser::GetRepoStatsFromJSON(*response_body, &repo_stats);
  std::move(callback).Run(success, repo_stats);
}

void IpfsService::GetNodeInfo(GetNodeInfoCallback callback) {
  if (!IsDaemonLaunched()) {
    std::move(callback).Run(false, NodeInfo());
    return;
  }

  GURL gurl = server_endpoint_.Resolve(ipfs::kNodeInfoPath);
  auto url_loader = CreateURLLoader(gurl);
  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));

  iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&IpfsService::OnNodeInfo, base::Unretained(this),
                     std::move(iter), std::move(callback)));
}

void IpfsService::OnNodeInfo(SimpleURLLoaderList::iterator iter,
                             GetNodeInfoCallback callback,
                             std::unique_ptr<std::string> response_body) {
  auto* url_loader = iter->get();
  int error_code = url_loader->NetError();
  int response_code = -1;
  if (url_loader->ResponseInfo() && url_loader->ResponseInfo()->headers)
    response_code = url_loader->ResponseInfo()->headers->response_code();
  url_loaders_.erase(iter);

  ipfs::NodeInfo node_info;
  if (error_code != net::OK || response_code != net::HTTP_OK) {
    VLOG(1) << "Fail to get node info, error_code = " << error_code
            << " response_code = " << response_code;
    std::move(callback).Run(false, node_info);
    return;
  }

  bool success =
      IPFSJSONParser::GetNodeInfoFromJSON(*response_body, &node_info);
  std::move(callback).Run(success, node_info);
}

void IpfsService::RunGarbageCollection(GarbageCollectionCallback callback) {
  if (!IsDaemonLaunched()) {
    std::move(callback).Run(false, std::string());
    return;
  }

  GURL gurl = server_endpoint_.Resolve(ipfs::kGarbageCollectionPath);

  auto url_loader = CreateURLLoader(gurl);
  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));

  iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&IpfsService::OnGarbageCollection, base::Unretained(this),
                     std::move(iter), std::move(callback)));
}

void IpfsService::OnGarbageCollection(
    SimpleURLLoaderList::iterator iter,
    GarbageCollectionCallback callback,
    std::unique_ptr<std::string> response_body) {
  auto* url_loader = iter->get();
  int error_code = url_loader->NetError();
  int response_code = -1;
  if (url_loader->ResponseInfo() && url_loader->ResponseInfo()->headers)
    response_code = url_loader->ResponseInfo()->headers->response_code();
  url_loaders_.erase(iter);

  bool success = (error_code == net::OK && response_code == net::HTTP_OK);
  if (!success) {
    VLOG(1) << "Fail to run garbage collection, error_code = " << error_code
            << " response_code = " << response_code;
  }

  std::string error;
  if (success) {
    const std::string& body = *response_body;
    if (!body.empty())
      success = IPFSJSONParser::GetGarbageCollectionFromJSON(body, &error);
  }
  std::move(callback).Run(success && error.empty(), error);
}

}  // namespace ipfs
