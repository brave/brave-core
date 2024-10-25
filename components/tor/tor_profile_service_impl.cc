/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/tor_profile_service_impl.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/task/thread_pool.h"
#include "brave/components/tor/brave_tor_pluggable_transport_updater.h"
#include "brave/components/tor/pref_names.h"
#include "brave/components/tor/tor_constants.h"
#include "brave/components/tor/tor_utils.h"
#include "brave/net/proxy_resolution/proxy_config_service_tor.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "net/base/network_anonymization_key.h"
#include "net/base/schemeful_site.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "net/url_request/url_request_context.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/proxy_lookup_client.mojom.h"

using content::BrowserContext;
using content::BrowserThread;
using content::NavigationController;
using content::WebContents;
using content::WebContentsObserver;

namespace tor {

namespace {

class NewTorCircuitTracker : public WebContentsObserver {
 public:
  explicit NewTorCircuitTracker(content::WebContents* web_contents)
      : WebContentsObserver(web_contents) {}
  NewTorCircuitTracker(const NewTorCircuitTracker&) = delete;
  NewTorCircuitTracker& operator=(const NewTorCircuitTracker&) = delete;
  ~NewTorCircuitTracker() override = default;

  void NewIdentityLoaded(bool success) {
    if (web_contents()) {
      if (success) {
        NavigationController& controller = web_contents()->GetController();
        controller.Reload(content::ReloadType::BYPASSING_CACHE, true);
      } else {
        LOG(WARNING) << "Failed to set new tor circuit";
        // TODO(bridiver) - the webcontents still exists so we need to notify
        // the user, not just log and return;
      }
    }
  }
};

class TorProxyLookupClient : public network::mojom::ProxyLookupClient {
 public:
  TorProxyLookupClient(const TorProxyLookupClient&) = delete;
  TorProxyLookupClient& operator=(const TorProxyLookupClient&) = delete;

  static mojo::PendingRemote<network::mojom::ProxyLookupClient>
  CreateTorProxyLookupClient(NewTorCircuitCallback callback) {
    auto* lookup_client = new TorProxyLookupClient(std::move(callback));
    return lookup_client->GetProxyLookupClient();
  }

 private:
  explicit TorProxyLookupClient(NewTorCircuitCallback callback)
      : callback_(std::move(callback)) {}

  ~TorProxyLookupClient() override { receiver_.reset(); }

  mojo::PendingRemote<network::mojom::ProxyLookupClient>
  GetProxyLookupClient() {
    mojo::PendingRemote<network::mojom::ProxyLookupClient> pending_remote =
        receiver_.BindNewPipeAndPassRemote();
    receiver_.set_disconnect_handler(
        base::BindOnce(&TorProxyLookupClient::OnProxyLookupComplete,
                       base::Unretained(this), net::ERR_ABORTED, std::nullopt));
    return pending_remote;
  }

  // network::mojom::ProxyLookupClient:
  void OnProxyLookupComplete(
      int32_t net_error,
      const std::optional<net::ProxyInfo>& proxy_info) override {
    std::move(callback_).Run(proxy_info);
    delete this;
  }

  NewTorCircuitCallback callback_;
  mojo::Receiver<network::mojom::ProxyLookupClient> receiver_{this};
};

void OnNewTorCircuit(std::unique_ptr<NewTorCircuitTracker> tracker,
                     const std::optional<net::ProxyInfo>& proxy_info) {
  tracker->NewIdentityLoaded(proxy_info.has_value() &&
                             !proxy_info->is_direct());
}

constexpr const char kTorBuiltinBridgesFetchUrl[] =
    "https://bridges.torproject.org/moat/circumvention/builtin";

constexpr net::NetworkTrafficAnnotationTag kTorBuiltinBridgesMoatAnnotation =
    net::DefineNetworkTrafficAnnotation("brave_tor_bridges", R"(
    semantics {
      sender:
        "Built-in Bridges Request"
      description:
        "This service sends requests to the Tor bridges server."
      trigger:
        "When user opens new Tor window, but no more than once a day."
      destination: WEBSITE
    }
    policy {
      cookies_allowed: NO
    })");

constexpr const size_t kMaxBodySize = 4 * 1024;

}  // namespace

class BuiltinBridgesRequest {
 public:
  using ResultCallback = base::OnceCallback<void(const base::Value::Dict&)>;

  static std::unique_ptr<BuiltinBridgesRequest> MaybeUpdateBuiltinBridges(
      content::BrowserContext* browser_context,
      PrefService* local_state,
      ResultCallback callback) {
    const base::Time last_request_time =
        local_state->GetTime(prefs::kBuiltinBridgesRequestTime);
    if (base::Time::Now() > last_request_time + base::Days(1)) {
      local_state->SetTime(prefs::kBuiltinBridgesRequestTime,
                           base::Time::Now());
      return base::WrapUnique(
          new BuiltinBridgesRequest(browser_context, std::move(callback)));
    }
    return nullptr;
  }

 private:
  BuiltinBridgesRequest(content::BrowserContext* browser_context,
                        ResultCallback callback)
      : url_loader_factory_(browser_context->GetDefaultStoragePartition()
                                ->GetURLLoaderFactoryForBrowserProcess()),
        result_callback_(std::move(callback)) {
    auto request = std::make_unique<network::ResourceRequest>();
    request->url = GURL(kTorBuiltinBridgesFetchUrl);
    request->method = net::HttpRequestHeaders::kGetMethod;
    request->credentials_mode = network::mojom::CredentialsMode::kOmit;
    request->load_flags = net::LOAD_DO_NOT_SAVE_COOKIES |
                          net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE;
    request->headers.SetHeader("Content-Type", "application/json");
    simple_url_loader_ = network::SimpleURLLoader::Create(
        std::move(request), kTorBuiltinBridgesMoatAnnotation);
    simple_url_loader_->DownloadToString(
        url_loader_factory_.get(),
        base::BindOnce(&BuiltinBridgesRequest::OnResponse,
                       weak_factory_.GetWeakPtr()),
        kMaxBodySize);
  }

  void OnResponse(std::optional<std::string> response_body) {
    simple_url_loader_.reset();
    if (!response_body) {
      OnDataParsed(base::unexpected("Request has failed."));
    } else {
      data_decoder::DataDecoder::ParseJsonIsolated(
          *response_body, base::BindOnce(&BuiltinBridgesRequest::OnDataParsed,
                                         weak_factory_.GetWeakPtr()));
    }
  }

  void OnDataParsed(data_decoder::DataDecoder::ValueOrError value) {
    if (!value.has_value() || !value->is_dict()) {
      return std::move(result_callback_).Run(base::Value::Dict());
    }
    std::move(result_callback_).Run(value->GetDict());
  }

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<network::SimpleURLLoader> simple_url_loader_;
  ResultCallback result_callback_;
  base::WeakPtrFactory<BuiltinBridgesRequest> weak_factory_{this};
};

TorProfileServiceImpl::TorProfileServiceImpl(
    content::BrowserContext* original_context,
    content::BrowserContext* context,
    PrefService* local_state,
    BraveTorClientUpdater* tor_client_updater,
    BraveTorPluggableTransportUpdater* tor_pluggable_transport_updater)
    : context_(context),
      local_state_(local_state),
      tor_client_updater_(tor_client_updater),
      tor_pluggable_transport_updater_(tor_pluggable_transport_updater),
      tor_launcher_factory_(TorLauncherFactory::GetInstance()),
      weak_ptr_factory_(this) {
  if (tor_launcher_factory_) {
    tor_launcher_factory_->AddObserver(this);
  }

  if (tor_client_updater_) {
    tor_client_updater_->AddObserver(this);
  }

  if (tor_pluggable_transport_updater_) {
    tor_pluggable_transport_updater_->AddObserver(this);
  }

  pref_change_registrar_.Init(local_state_.get());

  builtin_bridges_request_ = BuiltinBridgesRequest::MaybeUpdateBuiltinBridges(
      original_context, local_state,
      base::BindOnce(&TorProfileServiceImpl::OnBuiltinBridgesResponse,
                     weak_ptr_factory_.GetWeakPtr()));
}

TorProfileServiceImpl::~TorProfileServiceImpl() {
  if (tor_launcher_factory_) {
    tor_launcher_factory_->RemoveObserver(this);
  }

  if (tor_client_updater_) {
    tor_client_updater_->RemoveObserver(this);
  }

  if (tor_pluggable_transport_updater_) {
    tor_pluggable_transport_updater_->RemoveObserver(this);
  }
}

void TorProfileServiceImpl::OnExecutableReady(const base::FilePath& path) {
  if (path.empty()) {
    return;
  }

  if (tor_launcher_factory_->GetTorPid() < 0) {
    LaunchTor();
  }
}

void TorProfileServiceImpl::OnPluggableTransportReady(bool success) {
  if (!success || !tor_launcher_factory_) {
    return;
  }

#if DCHECK_IS_ON()
  // Check we can touch pluggable transport executables from the tor's working
  // dir.
  const auto snowflake_path = base::FilePath::FromASCII("../../").Append(
      tor_pluggable_transport_updater_->GetSnowflakeExecutable());
  const auto obfs4_path = base::FilePath::FromASCII("../../").Append(
      tor_pluggable_transport_updater_->GetObfs4Executable());
  tor_pluggable_transport_updater_->GetTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](const base::FilePath& tor, const base::FilePath& snowflake_path,
             const base::FilePath& obfs4_path) {
            if (tor.empty()) {
              return;
            }
            const auto tor_path = tor.DirName();
            DCHECK(base::PathExists(tor_path.Append(snowflake_path)));
            DCHECK(base::PathExists(tor_path.Append(obfs4_path)));
          },
          tor_client_updater_->executable(), snowflake_path, obfs4_path));
#endif

  OnBridgesConfigChanged();
}

void TorProfileServiceImpl::OnBridgesConfigChanged() {
  auto config = tor::BridgesConfig::FromDict(
                    local_state_->GetDict(tor::prefs::kBridgesConfig))
                    .value_or(tor::BridgesConfig());
  if (!tor_pluggable_transport_updater_) {
    return;
  }

  if (!tor_pluggable_transport_updater_->IsReady()) {
    if (config.use_bridges != tor::BridgesConfig::Usage::kNotUsed) {
      tor_pluggable_transport_updater_->Register();
      return;
    } else {
      tor_pluggable_transport_updater_->Unregister();
    }
  }
  if (config.use_bridges == tor::BridgesConfig::Usage::kNotUsed) {
    tor_pluggable_transport_updater_->Unregister();
  }

  tor_launcher_factory_->SetupPluggableTransport(
      tor_pluggable_transport_updater_->GetSnowflakeExecutable(),
      tor_pluggable_transport_updater_->GetObfs4Executable());
  tor_launcher_factory_->SetupBridges(std::move(config));
}

void TorProfileServiceImpl::OnBuiltinBridgesResponse(
    const base::Value::Dict& bridges) {
  if (!bridges.empty()) {
    auto config = tor::BridgesConfig::FromDict(
                      local_state_->GetDict(tor::prefs::kBridgesConfig))
                      .value_or(tor::BridgesConfig());
    config.UpdateBuiltinBridges(bridges);
    local_state_->SetDict(tor::prefs::kBridgesConfig, config.ToDict());
  }

  builtin_bridges_request_.reset();
}

void TorProfileServiceImpl::LaunchTor() {
  auto install_dir =
      base::SafeBaseName::Create(tor_client_updater_->install_dir());
  CHECK(install_dir) << tor_client_updater_->install_dir();

  auto executable =
      base::SafeBaseName::Create(tor_client_updater_->executable());
  CHECK(executable) << tor_client_updater_->executable();

  tor_launcher_factory_->LaunchTorProcess(tor::mojom::TorConfig(
      std::move(install_dir).value(), std::move(executable).value()));
}

void TorProfileServiceImpl::RegisterTorClientUpdater() {
  if (tor_client_updater_) {
    tor_client_updater_->Register();
  }
  if (tor_pluggable_transport_updater_) {
    pref_change_registrar_.Add(
        tor::prefs::kBridgesConfig,
        base::BindRepeating(&TorProfileServiceImpl::OnBridgesConfigChanged,
                            base::Unretained(this)));
    OnBridgesConfigChanged();
  }
}

void TorProfileServiceImpl::UnregisterTorClientUpdater() {
  if (tor_client_updater_) {
    tor_client_updater_->Unregister();
  }
  if (tor_pluggable_transport_updater_) {
    tor_pluggable_transport_updater_->Unregister();
  }
}

void TorProfileServiceImpl::SetNewTorCircuit(WebContents* tab) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  // track the webcontents lifetime so we don't reload if it has already
  // been destroyed
  auto tracker = std::make_unique<NewTorCircuitTracker>(tab);
  auto callback = base::BindOnce(&OnNewTorCircuit, std::move(tracker));

  const GURL& url = tab->GetURL();

  proxy_config_service_->SetNewTorCircuit(url);

  // Force lookup to erase the old circuit and also get a callback
  // so we know when it is safe to reload the tab
  auto* storage_partition = context_->GetStoragePartitionForUrl(url, false);
  if (!storage_partition) {
    storage_partition = context_->GetDefaultStoragePartition();
  }
  auto proxy_lookup_client =
      TorProxyLookupClient::CreateTorProxyLookupClient(std::move(callback));
  const net::SchemefulSite url_site(url);
  const auto network_anonymization_key =
      net::NetworkAnonymizationKey::CreateFromFrameSite(url_site, url_site);
  storage_partition->GetNetworkContext()->LookUpProxyForURL(
      url, network_anonymization_key, std::move(proxy_lookup_client));
}

void TorProfileServiceImpl::KillTor() {
  if (tor_launcher_factory_) {
    tor_launcher_factory_->KillTorProcess();
  }
  UnregisterTorClientUpdater();
}

void TorProfileServiceImpl::OnTorControlReady() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  OnBridgesConfigChanged();
}

void TorProfileServiceImpl::OnTorNewProxyURI(const std::string& uri) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(proxy_config_service_);
  proxy_config_service_->UpdateProxyURI(uri);
}

std::unique_ptr<net::ProxyConfigService>
TorProfileServiceImpl::CreateProxyConfigService() {
  // First tor profile will have empty proxy uri but it will receive update from
  // NotifyTorNewProxyURI. And subsequent tor profile might not have
  // NotifyTorNewProxyURI because it is called once when tor control is ready.
  const std::string tor_proxy_uri = tor_launcher_factory_->GetTorProxyURI();
  if (tor_proxy_uri.empty()) {
    proxy_config_service_ = new net::ProxyConfigServiceTor();
  } else {
    proxy_config_service_ = new net::ProxyConfigServiceTor(tor_proxy_uri);
  }
  return std::unique_ptr<net::ProxyConfigServiceTor>(proxy_config_service_);
}

bool TorProfileServiceImpl::IsTorConnected() {
  if (!tor_launcher_factory_) {
    return false;
  }
  return tor_launcher_factory_->IsTorConnected();
}

void TorProfileServiceImpl::SetTorLauncherFactoryForTest(
    TorLauncherFactory* factory) {
  if (!factory) {
    return;
  }
  tor_launcher_factory_ = factory;
}

}  // namespace tor
