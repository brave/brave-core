/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_service.h"

#include <iterator>
#include <memory>
#include <optional>
#include <set>
#include <utility>

#include "base/files/file_util.h"
#include "base/hash/hash.h"
#include "base/memory/raw_ref.h"
#include "base/rand_util.h"
#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "base/task/thread_pool.h"
#include "brave/base/process/process_launcher.h"
#include "brave/components/ipfs/blob_context_getter_factory.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_json_parser.h"
#include "brave/components/ipfs/ipfs_network_utils.h"
#include "brave/components/ipfs/ipfs_ports.h"
#include "brave/components/ipfs/ipfs_service_delegate.h"
#include "brave/components/ipfs/ipfs_service_observer.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/pref_names.h"
#include "brave/components/ipfs/service_sandbox_type.h"
#include "build/build_config.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "net/base/url_util.h"
#include "net/http/http_request_headers.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
#include "base/threading/thread_restrictions.h"
#include "brave/components/ipfs/import/ipfs_import_worker_base.h"
#include "brave/components/ipfs/import/ipfs_link_import_worker.h"
#include "brave/components/ipfs/keys/ipns_keys_manager.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/service_process_host.h"
#endif

namespace {
#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)

// Works similarly to base::AutoReset but checks for access from the wrong
// thread as well as ensuring that the previous value of the re-entrancy guard
// variable was false.
// TODO(cdesouza): Replace this class with base::AutoReset in M114, as it now
// offers a way to check the starting value, and as it also reduces maintainance
// burden.
class ReentrancyCheck {
 public:
  explicit ReentrancyCheck(bool& guard_flag) : guard_flag_(guard_flag) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    DCHECK(!*guard_flag_);
    *guard_flag_ = true;
  }

  ~ReentrancyCheck() { *guard_flag_ = false; }

 private:
  const raw_ref<bool> guard_flag_;
};
#endif
// Used to retry request if we got zero peers from ipfs service
// Actual value will be generated randomly in range
// (kMinimalPeersRetryIntervalMs, kPeersRetryRate*kMinimalPeersRetryIntervalMs)
const int kMinimalPeersRetryIntervalMs = 350;
const int kPeersRetryRate = 3;

const char kGatewayValidationCID[] = "bafkqae2xmvwgg33nmuqhi3zajfiemuzahiwss";
const char kGatewayValidationResult[] = "Welcome to IPFS :-)";

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

base::flat_map<std::string, std::string> GetHeaders(const GURL& url) {
  return {
      {net::HttpRequestHeaders::kOrigin, url::Origin::Create(url).Serialize()}};
}

std::optional<std::string> ConvertPlainStringToJsonArray(
    const std::string& json) {
  return base::StrCat({"[\"", json, "\"]"});
}

#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
// Contains blessed extension IDs for all channels
inline constexpr const char* kBlessedExtensionIds_Stable[] = {
    // WebRecorder
    "chrome-extension://fpeoodllldobpkbkabpblcfaogecpndd"};

// Contains blessed extension IDs for nigtly channel and previous ones
inline constexpr const char* kBlessedExtensionIds_Nightly[] = {
    // markdown-publish
    "chrome-extension://ioajeblglaafjfaepefmbohjlncbaaof",
    // link-list
    "chrome-extension://beppdjjojnaodnioccaagpgngahdejnk"};

std::vector<std::string> GetBlessedExtensionListForChannel(
    version_info::Channel channel) {
  std::vector<std::string> blessed_extensions_list(
      std::begin(kBlessedExtensionIds_Stable),
      std::end(kBlessedExtensionIds_Stable));

  if (channel <= version_info::Channel::CANARY) {
    blessed_extensions_list.insert(blessed_extensions_list.end(),
                                   std::begin(kBlessedExtensionIds_Nightly),
                                   std::end(kBlessedExtensionIds_Nightly));
  }

  return blessed_extensions_list;
}
#endif  // BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)

}  // namespace

namespace ipfs {

IpfsService::IpfsService() : ipfs_p3a_(nullptr, nullptr), weak_factory_(this) {}

IpfsService::IpfsService(
    PrefService* prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    BlobContextGetterFactoryPtr blob_context_getter_factory,
    const base::FilePath& user_data_dir,
    version_info::Channel channel,
    std::unique_ptr<ipfs::IpfsDnsResolver> ipfs_dns_resover,
    std::unique_ptr<IpfsServiceDelegate> ipfs_service_delegate)
    : prefs_(prefs),
      pref_change_registrar_(std::make_unique<PrefChangeRegistrar>()),
      url_loader_factory_(url_loader_factory),
      blob_context_getter_factory_(std::move(blob_context_getter_factory)),
      server_endpoint_(GetAPIServer(channel)),
      user_data_dir_(user_data_dir),
      channel_(channel),
      ipfs_dns_resolver_(std::move(ipfs_dns_resover)),
      file_task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
      ipfs_p3a_(this, prefs),
      ipfs_service_delegate_(std::move(ipfs_service_delegate)) {
  DCHECK(!user_data_dir.empty());

  api_request_helper_ = std::make_unique<api_request_helper::APIRequestHelper>(
      GetIpfsNetworkTrafficAnnotationTag(), url_loader_factory);

#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
  DCHECK(blob_context_getter_factory_);
  ipns_keys_manager_ = std::make_unique<IpnsKeysManager>(
      *blob_context_getter_factory_, url_loader_factory, server_endpoint_);
  AddObserver(ipns_keys_manager_.get());
#endif

  ipfs_dns_resolver_subscription_ =
      ipfs_dns_resolver_->AddObserver(base::BindRepeating(
          &IpfsService::OnDnsConfigChanged, weak_factory_.GetWeakPtr()));

  if (prefs_) {
    pref_change_registrar_->Init(prefs_);
    pref_change_registrar_->Add(
        kIPFSAlwaysStartMode,
        base::BindRepeating(&IpfsService::OnIPFSAlwaysStartModeChanged,
                            weak_factory_.GetWeakPtr()));
  }

  OnIPFSAlwaysStartModeChanged();
}

IpfsService::~IpfsService() {
#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
  if (observers_.HasObserver(ipns_keys_manager_.get())) {
    RemoveObserver(ipns_keys_manager_.get());
  }
#endif
  Shutdown();
}

// static
void IpfsService::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kIPFSEnabled, true);
  registry->RegisterIntegerPref(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_ASK));
  registry->RegisterBooleanPref(kIPFSAutoFallbackToGateway, false);
  registry->RegisterBooleanPref(kIPFSAlwaysStartMode, false);

  registry->RegisterBooleanPref(kIPFSLocalNodeUsed, false);
  registry->RegisterIntegerPref(kIPFSInfobarCount, 0);
  registry->RegisterIntegerPref(kIpfsStorageMax, 1);
  registry->RegisterStringPref(kIPFSPublicGatewayAddress, kDefaultIPFSGateway);
  registry->RegisterStringPref(kIPFSPublicNFTGatewayAddress,
                               kDefaultIPFSNFTGateway);
  registry->RegisterFilePathPref(kIPFSBinaryPath, base::FilePath());
  registry->RegisterDictionaryPref(kIPFSPinnedCids);
  registry->RegisterBooleanPref(kShowIPFSPromoInfobar, true);
  registry->RegisterBooleanPref(kIPFSAlwaysStartInfobarShown, false);

  // Deprecated, kIPFSAutoRedirectToConfiguredGateway is used instead
  registry->RegisterBooleanPref(kIPFSAutoRedirectGateway, false);
  registry->RegisterBooleanPref(kIPFSAutoRedirectDNSLink, false);
}

base::FilePath IpfsService::GetIpfsExecutablePath() const {
  return prefs_->GetFilePath(kIPFSBinaryPath);
}

std::string IpfsService::GetStorageSize() {
  return std::to_string(prefs_->GetInteger(kIpfsStorageMax)) + "GB";
}

void IpfsService::LaunchIfNotRunning(const base::FilePath& executable_path) {
#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
  if (ipfs_service_.is_bound())
    return;

  content::ServiceProcessHost::Launch(
      ipfs_service_.BindNewPipeAndPassReceiver(),
      content::ServiceProcessHost::Options()
          .WithDisplayName(IDS_UTILITY_PROCESS_IPFS_NAME)
          .Pass());

  ipfs_service_.set_disconnect_handler(
      base::BindOnce(&IpfsService::OnIpfsCrashed, weak_factory_.GetWeakPtr()));
  ipfs_service_->SetCrashHandler(base::BindOnce(
      &IpfsService::OnIpfsDaemonCrashed, weak_factory_.GetWeakPtr()));

  auto config = mojom::IpfsConfig::New(
      executable_path, GetConfigFilePath(), GetDataPath(),
      GetGatewayPort(channel_), GetAPIPort(channel_), GetSwarmPort(channel_),
      GetStorageSize(), ipfs_dns_resolver_->GetFirstDnsOverHttpsServer(),
      GetBlessedExtensionListForChannel(channel_));

  ipfs_service_->Launch(
      std::move(config),
      base::BindOnce(&IpfsService::OnIpfsLaunched, weak_factory_.GetWeakPtr()));
#endif
}

void IpfsService::RestartDaemon() {
  if (!IsDaemonLaunched())
    return;
  auto launch_callback =
      base::BindOnce(&IpfsService::LaunchDaemon, weak_factory_.GetWeakPtr());
  ShutdownDaemon(base::BindOnce(
      [](base::OnceCallback<void(BoolCallback)> launch_callback,
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

void IpfsService::OnIPFSAlwaysStartModeChanged() {
  const auto is_ipfs_local =
      (prefs_ &&
       prefs_->GetInteger(kIPFSResolveMethod) ==
           static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL));
  if (is_ipfs_local && prefs_ && prefs_->GetBoolean(kIPFSAlwaysStartMode)) {
    StartDaemonAndLaunch(base::NullCallback());
  }
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
#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
  if (success && ipns_keys_manager_) {
    ipns_keys_manager_->LoadKeys(base::BindOnce(
        &IpfsService::NotifyIpnsKeysLoaded, weak_factory_.GetWeakPtr()));
  }
#endif
  while (!pending_launch_callbacks_.empty()) {
    if (pending_launch_callbacks_.front())
      std::move(pending_launch_callbacks_.front()).Run(success);
    pending_launch_callbacks_.pop();
  }
  for (auto& observer : observers_) {
    observer.OnIpfsLaunched(result, pid);
  }
}

void IpfsService::OnIpfsLaunched(bool result, int64_t pid) {
  if (result) {
    ipfs_pid_ = pid;
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

void IpfsService::OnDnsConfigChanged(std::optional<std::string> dns_server) {
  RestartDaemon();
}

#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
// static
std::optional<std::string> IpfsService::WaitUntilExecutionFinished(
    base::FilePath data_path,
    base::CommandLine command_line) {
  base::LaunchOptions options;
#if BUILDFLAG(IS_WIN)
  options.environment[L"IPFS_PATH"] = data_path.value();
#else
  options.environment["IPFS_PATH"] = data_path.value();
#endif

#if BUILDFLAG(IS_LINUX)
  options.kill_on_parent_death = true;
#endif
#if BUILDFLAG(IS_WIN)
  options.start_hidden = true;
#endif
  return brave::ProcessLauncher::ReadAppOutput(command_line, options, 10);
}

void IpfsService::RotateKey(const std::string& oldkey, BoolCallback callback) {
  auto executable_path = GetIpfsExecutablePath();
  if (IsDaemonLaunched() || executable_path.empty()) {
    if (callback)
      std::move(callback).Run(false);
    return;
  }
  base::CommandLine cmdline(executable_path);
  cmdline.AppendArg("key");
  cmdline.AppendArg("rotate");
  cmdline.AppendArg("--oldkey=" + oldkey);
  ExecuteNodeCommand(
      cmdline, GetDataPath(),
      base::BindOnce(
          [](BoolCallback callback, std::optional<std::string> result) {
            std::move(callback).Run(result.has_value());
          },
          std::move(callback)));
}

void IpfsService::ExportKey(const std::string& key,
                            const base::FilePath& target_path,
                            BoolCallback callback) {
  base::FilePath path = GetIpfsExecutablePath();
  if (path.empty())
    return;

  base::CommandLine cmdline(path);
  cmdline.AppendArg("key");
  cmdline.AppendArg("export");
  cmdline.AppendArg("-o=" + target_path.MaybeAsASCII());
  cmdline.AppendArg(key);
  ExecuteNodeCommand(
      cmdline, GetDataPath(),
      base::BindOnce(
          [](BoolCallback callback, std::optional<std::string> result) {
            std::move(callback).Run(result.has_value());
          },
          std::move(callback)));
}

void IpfsService::ExecuteNodeCommand(const base::CommandLine& command_line,
                                     const base::FilePath& data,
                                     NodeCallback callback) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN,
       base::TaskPriority::BEST_EFFORT},
      base::BindOnce(&IpfsService::WaitUntilExecutionFinished, GetDataPath(),
                     command_line),
      std::move(callback));
}

void IpfsService::NotifyIpnsKeysLoaded(bool result) {
  for (auto& observer : observers_) {
    observer.OnIpnsKeysLoaded(result);
  }
}

void IpfsService::RemovePinCli(std::set<std::string> cids,
                               BoolCallback callback) {
  if (cids.empty()) {
    std::move(callback).Run(true);
    return;
  }

  base::FilePath path = GetIpfsExecutablePath();
  if (path.empty()) {
    std::move(callback).Run(false);
    return;
  }

  auto cid = *(cids.begin());

  if (!IsValidCID(cid)) {
    std::move(callback).Run(false);
    return;
  }

  base::CommandLine cmdline(path);
  cmdline.AppendArg("pin");
  cmdline.AppendArg("rm");
  cmdline.AppendArg("-r=true");

  cmdline.AppendArg(cid);
  ExecuteNodeCommand(
      cmdline, GetDataPath(),
      base::BindOnce(&IpfsService::OnRemovePinCli, weak_factory_.GetWeakPtr(),
                     std::move(callback), std::move(cids)));
}

void IpfsService::LsPinCli(NodeCallback callback) {
  base::FilePath path = GetIpfsExecutablePath();
  if (path.empty()) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  base::CommandLine cmdline(path);
  cmdline.AppendArg("pin");
  cmdline.AppendArg("ls");
  cmdline.AppendArg("--type=recursive");
  cmdline.AppendArg("--quiet=true");

  ExecuteNodeCommand(cmdline, GetDataPath(), std::move(callback));
}

void IpfsService::OnRemovePinCli(BoolCallback callback,
                                 std::set<std::string> cids,
                                 std::optional<std::string> result) {
  DCHECK(!cids.empty());
  if (!result || cids.empty()) {
    std::move(callback).Run(false);
    return;
  }

  cids.erase(cids.begin());

  if (cids.empty()) {
    std::move(callback).Run(true);
  } else {
    RemovePinCli(cids, std::move(callback));
  }
}

void IpfsService::ImportFileToIpfs(const base::FilePath& path,
                                   const std::string& key,
                                   ipfs::ImportCompletedCallback callback) {
  if (path.empty()) {
    if (callback)
      std::move(callback).Run(ipfs::ImportedData());
    return;
  }
  ReentrancyCheck reentrancy_check(reentrancy_guard_);
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
      blob_context_getter_factory_.get(), url_loader_factory_.get(),
      server_endpoint_, std::move(import_completed_callback), key);
  importers_[hash]->ImportFile(path);
}

void IpfsService::ImportLinkToIpfs(const GURL& url,
                                   ipfs::ImportCompletedCallback callback) {
  if (!url.is_valid()) {
    if (callback)
      std::move(callback).Run(ipfs::ImportedData());
    return;
  }

  ReentrancyCheck reentrancy_check(reentrancy_guard_);
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
      blob_context_getter_factory_.get(), url_loader_factory_.get(),
      server_endpoint_, std::move(import_completed_callback), url);
}

void IpfsService::ImportDirectoryToIpfs(const base::FilePath& folder,
                                        const std::string& key,
                                        ImportCompletedCallback callback) {
  if (folder.empty()) {
    if (callback)
      std::move(callback).Run(ipfs::ImportedData());
    return;
  }
  ReentrancyCheck reentrancy_check(reentrancy_guard_);
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
      blob_context_getter_factory_.get(), url_loader_factory_.get(),
      server_endpoint_, std::move(import_completed_callback), key);
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
  ReentrancyCheck reentrancy_check(reentrancy_guard_);
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
      blob_context_getter_factory_.get(), url_loader_factory_.get(),
      server_endpoint_, std::move(import_completed_callback));

  importers_[hash]->ImportText(text, host);
}

void IpfsService::OnImportFinished(ipfs::ImportCompletedCallback callback,
                                   size_t key,
                                   const ipfs::ImportedData& data) {
  bool is_import_success{data.state == ipfs::IPFS_IMPORT_SUCCESS};

  if (callback)
    std::move(callback).Run(data);

  importers_.erase(key);

  if (is_import_success && ipfs_service_delegate_) {
    ipfs_service_delegate_->OnImportToIpfsFinished(this);
  }
}
#endif

void IpfsService::GetConnectedPeers(GetConnectedPeersCallback callback,
                                    std::optional<int> retries) {
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
  auto gurl = server_endpoint_.Resolve(kSwarmPeersPath);
  api_request_helper_->Request(
      "POST", gurl, std::string(), std::string(),
      base::BindOnce(&IpfsService::OnGetConnectedPeers,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     retries.value_or(kPeersDefaultRetries)),
      GetHeaders(gurl));
}

base::TimeDelta IpfsService::CalculatePeersRetryTime() {
  if (zero_peer_time_for_test_)
    return base::TimeDelta();
  return base::Milliseconds(
      base::RandInt(kMinimalPeersRetryIntervalMs,
                    kPeersRetryRate * kMinimalPeersRetryIntervalMs));
}

void IpfsService::OnGetConnectedPeers(
    GetConnectedPeersCallback callback,
    int retry_number,
    api_request_helper::APIRequestResult response) {
  int response_code = response.response_code();

  bool success = response.Is2XXResponseCode();
  last_peers_retry_value_for_test_ = retry_number;
  if (response.error_code() == net::ERR_CONNECTION_REFUSED && retry_number) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&IpfsService::GetConnectedPeers,
                       weak_factory_.GetWeakPtr(), std::move(callback),
                       retry_number - 1),
        CalculatePeersRetryTime());
    return;
  }

  std::vector<std::string> peers;
  if (!success) {
    VLOG(1) << "Fail to get connected peers, response_code = " << response_code;
  }

  if (success)
    success = IPFSJSONParser::GetPeersFromJSON(response.value_body(), &peers);

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
  api_request_helper_->Request(
      "POST", gurl, std::string(), std::string(),
      base::BindOnce(&IpfsService::OnGetAddressesConfig,
                     weak_factory_.GetWeakPtr(), std::move(callback)),
      GetHeaders(gurl));
}

void IpfsService::OnGetAddressesConfig(
    GetAddressesConfigCallback callback,
    api_request_helper::APIRequestResult response) {
  int response_code = response.response_code();
  bool success = response.Is2XXResponseCode();

  ipfs::AddressesConfig addresses_config;
  if (!success) {
    VLOG(1) << "Fail to get addresses config, response_code = "
            << response_code;
    std::move(callback).Run(false, addresses_config);
    return;
  }

  success = IPFSJSONParser::GetAddressesConfigFromJSON(response.value_body(),
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
    if (success_callback) {
      std::move(success_callback).Run();
    }
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

void IpfsService::LaunchDaemon(BoolCallback callback) {
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
  } else {
    LaunchIfNotRunning(path);
  }
}

void IpfsService::ShutdownDaemon(BoolCallback callback) {
  if (IsDaemonLaunched()) {
    Shutdown();
  }

  for (auto& observer : observers_) {
    observer.OnIpfsShutdown();
  }

  if (callback)
    std::move(callback).Run(!IsDaemonLaunched());
}

void IpfsService::GetConfig(GetConfigCallback callback) {
  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
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
  return static_cast<IPFSResolveMethodTypes>(
      prefs_->GetInteger(kIPFSResolveMethod));
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
  api_request_helper_->Request(
      "POST", gurl, std::string(), std::string(),
      base::BindOnce(&IpfsService::OnRepoStats, weak_factory_.GetWeakPtr(),
                     std::move(callback)),
      GetHeaders(gurl));
}

void IpfsService::OnRepoStats(GetRepoStatsCallback callback,
                              api_request_helper::APIRequestResult response) {
  int response_code = response.response_code();

  bool success = response.Is2XXResponseCode();

  ipfs::RepoStats repo_stats;
  if (!success) {
    VLOG(1) << "Fail to get repro stats, response_code = " << response_code;
    std::move(callback).Run(false, repo_stats);
    return;
  }

  success =
      IPFSJSONParser::GetRepoStatsFromJSON(response.value_body(), &repo_stats);
  std::move(callback).Run(success, repo_stats);
}

void IpfsService::GetNodeInfo(GetNodeInfoCallback callback) {
  if (!IsDaemonLaunched()) {
    std::move(callback).Run(false, NodeInfo());
    return;
  }

  GURL gurl = server_endpoint_.Resolve(ipfs::kNodeInfoPath);

  api_request_helper_->Request(
      "POST", gurl, std::string(), std::string(),
      base::BindOnce(&IpfsService::OnNodeInfo, weak_factory_.GetWeakPtr(),
                     std::move(callback)),
      GetHeaders(gurl));
}

void IpfsService::OnNodeInfo(GetNodeInfoCallback callback,
                             api_request_helper::APIRequestResult response) {
  int response_code = response.response_code();

  bool success = response.Is2XXResponseCode();

  ipfs::NodeInfo node_info;
  if (!success) {
    VLOG(1) << "Fail to get node info, response_code = " << response_code;
    std::move(callback).Run(false, node_info);
    return;
  }

  success =
      IPFSJSONParser::GetNodeInfoFromJSON(response.value_body(), &node_info);
  std::move(callback).Run(success, node_info);
}

void IpfsService::RunGarbageCollection(GarbageCollectionCallback callback) {
  if (!IsDaemonLaunched()) {
    std::move(callback).Run(false, std::string());
    return;
  }

  GURL gurl = server_endpoint_.Resolve(ipfs::kGarbageCollectionPath);

  api_request_helper_->Request(
      "POST", gurl, std::string(), std::string(),
      base::BindOnce(&IpfsService::OnGarbageCollection,
                     weak_factory_.GetWeakPtr(), std::move(callback)),
      GetHeaders(gurl));
}

void IpfsService::OnGarbageCollection(
    GarbageCollectionCallback callback,
    api_request_helper::APIRequestResult response) {
  int response_code = response.response_code();

  bool success = response.Is2XXResponseCode();
  if (!success) {
    VLOG(1) << "Fail to run garbage collection, response_code = "
            << response_code;
  }

  std::string error;
  if (success) {
    IPFSJSONParser::GetGarbageCollectionFromJSON(response.value_body(), &error);
  }
  std::move(callback).Run(success && error.empty(), error);
}

void IpfsService::PreWarmShareableLink(const GURL& url) {
  api_request_helper_->Request("HEAD", url, std::string(), std::string(),
                               base::BindOnce(&IpfsService::OnPreWarmComplete,
                                              weak_factory_.GetWeakPtr()),
                               GetHeaders(url));
}

void IpfsService::OnPreWarmComplete(
    [[maybe_unused]] api_request_helper::APIRequestResult response) {
  if (prewarm_callback_for_testing_)
    std::move(prewarm_callback_for_testing_).Run();
}


void IpfsService::ValidateGateway(const GURL& url, BoolCallback callback) {
  GURL::Replacements replacements;
  std::string path = "/ipfs/";
  path += kGatewayValidationCID;
  replacements.SetPathStr(path);
  GURL validation_url = url.ReplaceComponents(replacements);

  auto conversion_callback = base::BindOnce(&ConvertPlainStringToJsonArray);
  api_request_helper_->Request(
      "GET", validation_url, "", "",
      base::BindOnce(&IpfsService::OnGatewayValidationComplete,
                     weak_factory_.GetWeakPtr(), std::move(callback), url),
      {}, {}, std::move(conversion_callback));
}

void IpfsService::OnGatewayValidationComplete(
    BoolCallback callback,
    const GURL& initial_url,
    api_request_helper::APIRequestResult response) const {
  int response_code = response.response_code();

  bool success = response.Is2XXResponseCode();
  if (!success) {
    VLOG(1) << "Fail to validate gateway, response_code = " << response_code;
  }

  std::string error;
  if (success) {
    const auto final_url = response.final_url();
    const std::string valid_host = base::StringPrintf(
        "%s.ipfs.%s", kGatewayValidationCID, initial_url.host().c_str());
    success = (response.SerializeBodyToString() ==
               ConvertPlainStringToJsonArray(kGatewayValidationResult)) &&
              (initial_url.host() != final_url.host()) &&
              (initial_url.scheme() == final_url.scheme()) &&
              (final_url.host() == valid_host);
  }

  if (callback)
    std::move(callback).Run(success);
}

}  // namespace ipfs
