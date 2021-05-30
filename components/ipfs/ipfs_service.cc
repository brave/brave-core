/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_service.h"

#include <utility>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/process/launch.h"
#include "base/process/process.h"
#include "base/rand_util.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "base/task_runner_util.h"
#include "brave/components/ipfs/import/ipfs_import_worker_base.h"
#include "brave/components/ipfs/import/ipfs_link_import_worker.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_json_parser.h"
#include "brave/components/ipfs/ipfs_ports.h"
#include "brave/components/ipfs/ipfs_service_observer.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/keys/ipns_keys_manager.h"
#include "brave/components/ipfs/pref_names.h"
#include "brave/components/ipfs/service_sandbox_type.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
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

// Works similarly to base::AutoReset but checks for access from the wrong
// thread as well as ensuring that the previous value of the re-entrancy guard
// variable was false.
class ReentrancyCheck {
 public:
  explicit ReentrancyCheck(bool* guard_flag) : guard_flag_(guard_flag) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    DCHECK(!*guard_flag_);
    *guard_flag_ = true;
  }

  ~ReentrancyCheck() { *guard_flag_ = false; }

 private:
  bool* const guard_flag_;
};

// Used to retry request if we got zero peers from ipfs service
// Actual value will be generated randomly in range
// (kMinimalPeersRetryIntervalMs, kPeersRetryRate*kMinimalPeersRetryIntervalMs)
const int kMinimalPeersRetryIntervalMs = 350;
const int kPeersRetryRate = 3;

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
      file_task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
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
  ipns_keys_manager_ =
      std::make_unique<IpnsKeysManager>(context_, server_endpoint_);
  AddObserver(ipns_keys_manager_.get());
}

IpfsService::~IpfsService() {
  if (ipfs_client_updater_) {
    ipfs_client_updater_->RemoveObserver(this);
  }
  RemoveObserver(ipns_keys_manager_.get());
  Shutdown();
}

// static
void IpfsService::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kIPFSEnabled, true);
  registry->RegisterIntegerPref(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_ASK));
  registry->RegisterBooleanPref(kIPFSAutoFallbackToGateway, false);
  registry->RegisterBooleanPref(kIPFSAutoRedirectGateway, false);
  registry->RegisterBooleanPref(kIPFSAutoRedirectDNSLink, false);
  registry->RegisterIntegerPref(kIPFSInfobarCount, 0);
  registry->RegisterIntegerPref(kIpfsStorageMax, 1);
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

std::string IpfsService::GetStorageSize() {
  PrefService* prefs = user_prefs::UserPrefs::Get(context_);
  return std::to_string(prefs->GetInteger(kIpfsStorageMax)) + "GB";
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
  ipfs_service_->SetCrashHandler(base::BindOnce(
      &IpfsService::OnIpfsDaemonCrashed, base::Unretained(this)));

  auto config = mojom::IpfsConfig::New(
      executable_path, GetConfigFilePath(), GetDataPath(),
      GetGatewayPort(channel_), GetAPIPort(channel_), GetSwarmPort(channel_),
      GetStorageSize());

  ipfs_service_->Launch(
      std::move(config),
      base::BindOnce(&IpfsService::OnIpfsLaunched, base::Unretained(this)));
}

void IpfsService::RestartDaemon() {
  if (!IsDaemonLaunched())
    return;
  auto launch_callback =
      base::BindOnce(&IpfsService::LaunchDaemon, base::Unretained(this));
  ShutdownDaemon(base::BindOnce(
      [](base::OnceCallback<void(LaunchDaemonCallback)> launch_callback,
         const bool success) {
        if (!success) {
          VLOG(1) << "Unable to shutdown daemon";
          return;
        }
        if (launch_callback) {
          std::move(launch_callback).Run(base::NullCallback());
        }
      },
      std::move(launch_callback)));
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

void IpfsService::NotifyDaemonLaunched(bool result, int64_t pid) {
  bool success = result && pid > 0;
  if (success && ipns_keys_manager_) {
    ipns_keys_manager_->LoadKeys(base::BindOnce(
        &IpfsService::NotifyIpnsKeysLoaded, weak_factory_.GetWeakPtr()));
  }

  while (!pending_launch_callbacks_.empty()) {
    if (pending_launch_callbacks_.front())
      std::move(pending_launch_callbacks_.front()).Run(success);
    pending_launch_callbacks_.pop();
  }
  for (auto& observer : observers_) {
    observer.OnIpfsLaunched(result, pid);
  }
}

void IpfsService::NotifyIpnsKeysLoaded(bool result) {
  for (auto& observer : observers_) {
    observer.OnIpnsKeysLoaded(result);
  }
}

void IpfsService::OnIpfsLaunched(bool result, int64_t pid) {
  if (result) {
    ipfs_pid_ = pid;
    RegisterIpfsClientUpdater();
  } else {
    VLOG(0) << "Failed to launch IPFS";
    Shutdown();
  }
  NotifyDaemonLaunched(result, pid);
}

void IpfsService::Shutdown() {
  if (ipfs_service_.is_bound()) {
    ipfs_service_->Shutdown();
  }
  ipfs_service_.reset();
  ipfs_pid_ = -1;
}

void IpfsService::ImportFileToIpfs(const base::FilePath& path,
                                   const std::string& key,
                                   ipfs::ImportCompletedCallback callback) {
  if (path.empty()) {
    if (callback)
      std::move(callback).Run(ipfs::ImportedData());
    return;
  }
  ReentrancyCheck reentrancy_check(&reentrancy_guard_);
  if (!IsDaemonLaunched()) {
    StartDaemonAndLaunch(base::BindOnce(&IpfsService::ImportFileToIpfs,
                                        weak_factory_.GetWeakPtr(), path, key,
                                        std::move(callback)));
    return;
  }
  size_t hash = base::FastHash(base::as_bytes(base::make_span(path.value())));
  if (importers_.count(hash))
    return;

  auto import_completed_callback =
      base::BindOnce(&IpfsService::OnImportFinished, weak_factory_.GetWeakPtr(),
                     std::move(callback), hash);
  importers_[hash] = std::make_unique<IpfsImportWorkerBase>(
      context_, server_endpoint_, std::move(import_completed_callback), key);
  importers_[hash]->ImportFile(path);
}

void IpfsService::ImportLinkToIpfs(const GURL& url,
                                   ipfs::ImportCompletedCallback callback) {
  if (!url.is_valid()) {
    if (callback)
      std::move(callback).Run(ipfs::ImportedData());
    return;
  }

  ReentrancyCheck reentrancy_check(&reentrancy_guard_);
  if (!IsDaemonLaunched()) {
    StartDaemonAndLaunch(base::BindOnce(&IpfsService::ImportLinkToIpfs,
                                        weak_factory_.GetWeakPtr(), url,
                                        std::move(callback)));
    return;
  }
  size_t hash = base::FastHash(base::as_bytes(base::make_span(url.spec())));
  if (importers_.count(hash))
    return;

  auto import_completed_callback =
      base::BindOnce(&IpfsService::OnImportFinished, weak_factory_.GetWeakPtr(),
                     std::move(callback), hash);
  importers_[hash] = std::make_unique<IpfsLinkImportWorker>(
      context_, server_endpoint_, std::move(import_completed_callback), url);
}

void IpfsService::ImportDirectoryToIpfs(const base::FilePath& folder,
                                        const std::string& key,
                                        ImportCompletedCallback callback) {
  if (folder.empty()) {
    if (callback)
      std::move(callback).Run(ipfs::ImportedData());
    return;
  }
  ReentrancyCheck reentrancy_check(&reentrancy_guard_);
  if (!IsDaemonLaunched()) {
    StartDaemonAndLaunch(base::BindOnce(&IpfsService::ImportDirectoryToIpfs,
                                        weak_factory_.GetWeakPtr(), folder, key,
                                        std::move(callback)));
    return;
  }
  size_t hash =
      base::FastHash(base::as_bytes(base::make_span(folder.MaybeAsASCII())));
  if (importers_.count(hash))
    return;
  auto import_completed_callback =
      base::BindOnce(&IpfsService::OnImportFinished, weak_factory_.GetWeakPtr(),
                     std::move(callback), hash);
  importers_[hash] = std::make_unique<IpfsImportWorkerBase>(
      context_, server_endpoint_, std::move(import_completed_callback), key);
  importers_[hash]->ImportFolder(folder);
}

void IpfsService::ImportTextToIpfs(const std::string& text,
                                   const std::string& host,
                                   ipfs::ImportCompletedCallback callback) {
  if (text.empty()) {
    if (callback)
      std::move(callback).Run(ipfs::ImportedData());
    return;
  }
  ReentrancyCheck reentrancy_check(&reentrancy_guard_);
  if (!IsDaemonLaunched()) {
    StartDaemonAndLaunch(base::BindOnce(&IpfsService::ImportTextToIpfs,
                                        weak_factory_.GetWeakPtr(), text, host,
                                        std::move(callback)));
    return;
  }
  size_t hash = base::FastHash(base::as_bytes(base::make_span(text)));
  if (importers_.count(hash))
    return;
  auto import_completed_callback =
      base::BindOnce(&IpfsService::OnImportFinished, weak_factory_.GetWeakPtr(),
                     std::move(callback), hash);
  importers_[hash] = std::make_unique<IpfsImportWorkerBase>(
      context_, server_endpoint_, std::move(import_completed_callback));

  importers_[hash]->ImportText(text, host);
}

void IpfsService::OnImportFinished(ipfs::ImportCompletedCallback callback,
                                   size_t key,
                                   const ipfs::ImportedData& data) {
  if (callback)
    std::move(callback).Run(data);

  importers_.erase(key);
}

void IpfsService::GetConnectedPeers(GetConnectedPeersCallback callback,
                                    int retries) {
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

  auto url_loader =
      CreateURLLoader(server_endpoint_.Resolve(kSwarmPeersPath), "POST");
  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));

  iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&IpfsService::OnGetConnectedPeers, base::Unretained(this),
                     iter, std::move(callback), retries));
}

base::TimeDelta IpfsService::CalculatePeersRetryTime() {
  if (zero_peer_time_for_test_)
    return base::TimeDelta();
  return base::TimeDelta::FromMilliseconds(
      base::RandInt(kMinimalPeersRetryIntervalMs,
                    kPeersRetryRate * kMinimalPeersRetryIntervalMs));
}

void IpfsService::OnGetConnectedPeers(
    SimpleURLLoaderList::iterator iter,
    GetConnectedPeersCallback callback,
    int retry_number,
    std::unique_ptr<std::string> response_body) {
  auto* url_loader = iter->get();
  int error_code = url_loader->NetError();
  int response_code = -1;
  if (url_loader->ResponseInfo() && url_loader->ResponseInfo()->headers)
    response_code = url_loader->ResponseInfo()->headers->response_code();
  url_loaders_.erase(iter);
  last_peers_retry_value_for_test_ = retry_number;
  if (error_code == net::ERR_CONNECTION_REFUSED && retry_number) {
    base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&IpfsService::GetConnectedPeers,
                       weak_factory_.GetWeakPtr(), std::move(callback),
                       retry_number - 1),
        CalculatePeersRetryTime());
    return;
  }

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
  auto url_loader = CreateURLLoader(gurl, "POST");
  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));

  iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&IpfsService::OnGetAddressesConfig, base::Unretained(this),
                     iter, std::move(callback)));
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

void IpfsService::StartDaemonAndLaunch(
    base::OnceCallback<void(void)> success_callback) {
  if (IsDaemonLaunched()) {
    std::move(success_callback).Run();
    return;
  }
  LaunchDaemon(base::BindOnce(
      [](base::OnceCallback<void(void)> launched_callback, bool success) {
        if (!success)
          return;
        if (launched_callback)
          std::move(launched_callback).Run();
      },
      std::move(success_callback)));
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
  if (callback)
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

int IpfsService::GetLastPeersRetryForTest() const {
  return last_peers_retry_value_for_test_;
}

void IpfsService::SetZeroPeersDeltaForTest(bool value) {
  zero_peer_time_for_test_ = value;
}

void IpfsService::SetAllowIpfsLaunchForTest(bool launched) {
  allow_ipfs_launch_for_test_ = launched;
}

void IpfsService::SetServerEndpointForTest(const GURL& gurl) {
  server_endpoint_ = gurl;
}

void IpfsService::RunLaunchDaemonCallbackForTest(bool result) {
  NotifyDaemonLaunched(result, 1);
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
  auto url_loader = CreateURLLoader(gurl, "POST");
  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));

  iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&IpfsService::OnRepoStats, base::Unretained(this), iter,
                     std::move(callback)));
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
  auto url_loader = CreateURLLoader(gurl, "POST");
  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));

  iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&IpfsService::OnNodeInfo, base::Unretained(this), iter,
                     std::move(callback)));
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

  auto url_loader = CreateURLLoader(gurl, "POST");
  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));

  iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&IpfsService::OnGarbageCollection, base::Unretained(this),
                     iter, std::move(callback)));
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
      IPFSJSONParser::GetGarbageCollectionFromJSON(body, &error);
  }
  std::move(callback).Run(success && error.empty(), error);
}

void IpfsService::PreWarmShareableLink(const GURL& url) {
  auto url_loader = CreateURLLoader(url, "HEAD");
  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));
  iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&IpfsService::OnPreWarmComplete, base::Unretained(this),
                     std::move(iter)));
}

void IpfsService::OnPreWarmComplete(
    SimpleURLLoaderList::iterator iter,
    std::unique_ptr<std::string> response_body) {
  url_loaders_.erase(iter);
  if (prewarm_callback_for_testing_)
    std::move(prewarm_callback_for_testing_).Run();
}

}  // namespace ipfs
