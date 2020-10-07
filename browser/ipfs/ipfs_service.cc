/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_service.h"

#include <utility>

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/json/json_reader.h"
#include "base/path_service.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/ipfs/ipfs_service_observer.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/common/brave_switches.h"
#include "brave/common/pref_names.h"
#include "brave/components/ipfs/browser/features.h"
#include "brave/components/ipfs/browser/ipfs_json_parser.h"
#include "brave/components/ipfs/common/ipfs_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/service_sandbox_type.h"
#include "chrome/common/chrome_paths.h"
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

}  // namespace

namespace ipfs {

IpfsService::IpfsService(content::BrowserContext* context)
  : context_(context),
    server_endpoint_(GURL(kServerEndpoint)) {
  // Return early since g_brave_browser_process and ipfs_client_updater are not
  // available in unit tests.
  if (Profile::FromBrowserContext(context)->AsTestingProfile()) {
    return;
  }

  url_loader_factory_ =
      content::BrowserContext::GetDefaultStoragePartition(context)
          ->GetURLLoaderFactoryForBrowserProcess();

  // TODO(jocelyn): Use /api/v0/repo/stat API to see if a remote daemon using
  // Brave's path is running (brave-ipfs), which is leftover from browser
  // crash, send a shutdown request if so.

  if (g_brave_browser_process)
    g_brave_browser_process->ipfs_client_updater()->AddObserver(this);
  OnExecutableReady(GetIpfsExecutablePath());
}

IpfsService::~IpfsService() = default;

// static
bool IpfsService::IsIpfsEnabled(content::BrowserContext* context) {
  if (!base::FeatureList::IsEnabled(features::kIpfsFeature) ||
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableIpfsClientUpdaterExtension))
    return false;

  // IPFS is disabled for OTR profiles, Tor profiles, and guest sessions.
  if (!brave::IsRegularProfile(context))
    return false;

  return true;
}

// static
void IpfsService::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_ASK));
  registry->RegisterBooleanPref(kIPFSBinaryAvailable, false);
}

base::FilePath IpfsService::GetIpfsExecutablePath() {
  return g_brave_browser_process->ipfs_client_updater()->GetExecutablePath();
}

void IpfsService::OnExecutableReady(const base::FilePath& path) {
  if (path.empty())
    return;

  PrefService* prefs = user_prefs::UserPrefs::Get(context_);
  prefs->SetBoolean(kIPFSBinaryAvailable, true);

  if (g_brave_browser_process)
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
    user_data_dir.Append(FILE_PATH_LITERAL("brave_ipfs"));
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

  if (!launch_daemon_callback_.is_null()) {
    std::move(launch_daemon_callback_).Run(result && pid > 0);
  }

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
    std::move(callback).Run(false, std::vector<std::string>{});
    return;
  }

  auto url_loader = CreateURLLoader(server_endpoint_.Resolve(kSwarmPeersPath));
  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));

  iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&IpfsService::OnGetConnectedPeers,
                     base::Unretained(this),
                     std::move(iter),
                     std::move(callback)));
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

  if (error_code != net::OK || response_code != net::HTTP_OK) {
    VLOG(1) << "Fail to get connected peers, error_code = " << error_code <<
        " response_code = " << response_code;
    std::move(callback).Run(false, std::vector<std::string>{});
    return;
  }

  std::vector<std::string> peers;
  bool success = IPFSJSONParser::GetPeersFromJSON(*response_body, &peers);
  std::move(callback).Run(success, peers);
}

void IpfsService::GetAddressesConfig(GetAddressesConfigCallback callback) {
  if (!IsDaemonLaunched()) {
    std::move(callback).Run(false, AddressesConfig());
    return;
  }

  GURL gurl = net::AppendQueryParameter(server_endpoint_.Resolve(kConfigPath),
                                        kArgQueryParam,
                                        kAddressesField);
  auto url_loader = CreateURLLoader(gurl);
  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));

  iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&IpfsService::OnGetAddressesConfig,
                     base::Unretained(this),
                     std::move(iter),
                     std::move(callback)));
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
    VLOG(1) << "Fail to get addresses config, error_code = " << error_code <<
        " response_code = " << response_code;
    std::move(callback).Run(false, addresses_config);
    return;
  }

  bool success = IPFSJSONParser::GetAddressesConfigFromJSON(
      *response_body,
      &addresses_config);
  std::move(callback).Run(success, addresses_config);
}

bool IpfsService::IsDaemonLaunched() const {
  if (is_ipfs_launched_for_test_)
    return true;

  return ipfs_pid_ > 0;
}

void IpfsService::LaunchDaemon(LaunchDaemonCallback callback) {
  // Reject if previous launch request in progress.
  if (launch_daemon_callback_) {
    std::move(callback).Run(false);
  }

  if (ipfs_pid_ > 0) {
    std::move(callback).Run(true);
  }

  base::FilePath path(GetIpfsExecutablePath());
  if (path.empty()) {  // Cannot launch if path is not ready.
    std::move(callback).Run(false);
    return;
  }

  launch_daemon_callback_ = std::move(callback);
  LaunchIfNotRunning(path);
}

void IpfsService::ShutdownDaemon(ShutdownDaemonCallback callback) {
  if (ipfs_pid_ > 0) {
    Shutdown();
  }

  std::move(callback).Run(true);
}

bool IpfsService::IsIPFSExecutableAvailable() const {
  PrefService* prefs = user_prefs::UserPrefs::Get(context_);
  return prefs->GetBoolean(kIPFSBinaryAvailable);
}

void IpfsService::AddObserver(IpfsServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void IpfsService::RemoveObserver(IpfsServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

void IpfsService::RegisterIpfsClientUpdater() {
  if (g_brave_browser_process)
    g_brave_browser_process->ipfs_client_updater()->Register();
}

void IpfsService::SetIpfsLaunchedForTest(bool launched) {
  is_ipfs_launched_for_test_ = launched;
}

void IpfsService::SetServerEndpointForTest(const GURL& gurl) {
  server_endpoint_ = gurl;
}

}  // namespace ipfs
