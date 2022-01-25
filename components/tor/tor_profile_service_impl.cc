/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/tor_profile_service_impl.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/task/post_task.h"
#include "brave/components/tor/pref_names.h"
#include "brave/components/tor/tor_constants.h"
#include "brave/net/proxy_resolution/proxy_config_service_tor.h"
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
#include "net/base/network_isolation_key.h"
#include "net/base/schemeful_site.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "net/url_request/url_request_context.h"
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
  ~NewTorCircuitTracker() override {}

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
        receiver_.BindNewPipeAndPassRemote(base::CreateSingleThreadTaskRunner(
            {content::BrowserThread::UI,
             content::BrowserTaskType::kPreconnect}));
    receiver_.set_disconnect_handler(base::BindOnce(
        &TorProxyLookupClient::OnProxyLookupComplete, base::Unretained(this),
        net::ERR_ABORTED, absl::nullopt));
    return pending_remote;
  }

  // network::mojom::ProxyLookupClient:
  void OnProxyLookupComplete(
      int32_t net_error,
      const absl::optional<net::ProxyInfo>& proxy_info) override {
    std::move(callback_).Run(proxy_info);
    delete this;
  }

  NewTorCircuitCallback callback_;
  mojo::Receiver<network::mojom::ProxyLookupClient> receiver_{this};
};

void OnNewTorCircuit(std::unique_ptr<NewTorCircuitTracker> tracker,
                     const absl::optional<net::ProxyInfo>& proxy_info) {
  tracker->NewIdentityLoaded(proxy_info.has_value() &&
                             !proxy_info->is_direct());
}

}  // namespace

TorProfileServiceImpl::TorProfileServiceImpl(
    content::BrowserContext* context,
    BraveTorClientUpdater* tor_client_updater)
    : context_(context),
      tor_client_updater_(tor_client_updater),
      tor_launcher_factory_(TorLauncherFactory::GetInstance()),
      weak_ptr_factory_(this) {
  if (tor_launcher_factory_) {
    tor_launcher_factory_->AddObserver(this);
  }

  if (tor_client_updater_) {
    tor_client_updater_->AddObserver(this);
  }
}

TorProfileServiceImpl::~TorProfileServiceImpl() {
  if (tor_launcher_factory_) {
    tor_launcher_factory_->RemoveObserver(this);
  }

  if (tor_client_updater_) {
    tor_client_updater_->RemoveObserver(this);
  }
}

void TorProfileServiceImpl::OnExecutableReady(const base::FilePath& path) {
  if (path.empty())
    return;

  if (tor_launcher_factory_->GetTorPid() < 0) {
    LaunchTor();
  }
}

void TorProfileServiceImpl::LaunchTor() {
  tor::mojom::TorConfig config(GetTorExecutablePath(), GetTorrcPath(),
                               GetTorDataPath(), GetTorWatchPath());
  tor_launcher_factory_->LaunchTorProcess(config);
}

base::FilePath TorProfileServiceImpl::GetTorExecutablePath() const {
  return tor_client_updater_ ? tor_client_updater_->GetExecutablePath()
                             : base::FilePath();
}

base::FilePath TorProfileServiceImpl::GetTorrcPath() const {
  return tor_client_updater_ ? tor_client_updater_->GetTorrcPath()
                             : base::FilePath();
}

base::FilePath TorProfileServiceImpl::GetTorDataPath() const {
  return tor_client_updater_ ? tor_client_updater_->GetTorDataPath()
                             : base::FilePath();
}

base::FilePath TorProfileServiceImpl::GetTorWatchPath() const {
  return tor_client_updater_ ? tor_client_updater_->GetTorWatchPath()
                             : base::FilePath();
}

void TorProfileServiceImpl::RegisterTorClientUpdater() {
  if (tor_client_updater_) {
    tor_client_updater_->Register();
  }
}

void TorProfileServiceImpl::UnregisterTorClientUpdater() {
  if (tor_client_updater_) {
    tor_client_updater_->Unregister();
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
  const net::NetworkIsolationKey network_isolation_key(url_site, url_site);
  storage_partition->GetNetworkContext()->LookUpProxyForURL(
      url, network_isolation_key, std::move(proxy_lookup_client));
}

void TorProfileServiceImpl::KillTor() {
  if (tor_launcher_factory_)
    tor_launcher_factory_->KillTorProcess();
  UnregisterTorClientUpdater();
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
  if (!tor_launcher_factory_)
    return false;
  return tor_launcher_factory_->IsTorConnected();
}

void TorProfileServiceImpl::SetTorLauncherFactoryForTest(
    TorLauncherFactory* factory) {
  if (!factory)
    return;
  tor_launcher_factory_ = factory;
}

}  // namespace tor
