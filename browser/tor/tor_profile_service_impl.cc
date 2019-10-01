/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_profile_service_impl.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/task/post_task.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/tor/tor_launcher_service_observer.h"
#include "brave/common/tor/pref_names.h"
#include "brave/common/tor/tor_constants.h"
#include "brave/net/proxy_resolution/proxy_config_service_tor.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
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

 private:
  DISALLOW_COPY_AND_ASSIGN(NewTorCircuitTracker);
};

class TorProxyLookupClient : public network::mojom::ProxyLookupClient {
 public:
  static network::mojom::ProxyLookupClientPtr CreateTorProxyLookupClient(
      NewTorCircuitCallback callback) {
    auto* lookup_client = new TorProxyLookupClient(std::move(callback));
    return lookup_client->GetProxyLookupClientPtr();
  }

 private:
  explicit TorProxyLookupClient(NewTorCircuitCallback callback)
      : callback_(std::move(callback)),
        binding_(this) {}

  ~TorProxyLookupClient() override {
    binding_.Close();
  }

  network::mojom::ProxyLookupClientPtr GetProxyLookupClientPtr() {
    network::mojom::ProxyLookupClientPtr proxy_lookup_client_ptr;
    binding_.Bind(
        mojo::MakeRequest(&proxy_lookup_client_ptr),
        base::CreateSingleThreadTaskRunnerWithTraits(
            {content::BrowserThread::UI,
             content::BrowserTaskType::kPreconnect}));
    binding_.set_connection_error_handler(
        base::BindOnce(&TorProxyLookupClient::OnProxyLookupComplete,
                       base::Unretained(this),
                       net::ERR_ABORTED,
                       base::nullopt));
    return proxy_lookup_client_ptr;
  }

  // network::mojom::ProxyLookupClient:
  void OnProxyLookupComplete(
        int32_t net_error,
        const base::Optional<net::ProxyInfo>& proxy_info) override {
    std::move(callback_).Run(proxy_info);
    delete this;
  }

  NewTorCircuitCallback callback_;
  mojo::Binding<network::mojom::ProxyLookupClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(TorProxyLookupClient);
};

void OnNewTorCircuit(std::unique_ptr<NewTorCircuitTracker> tracker,
                            const base::Optional<net::ProxyInfo>& proxy_info) {
  tracker->NewIdentityLoaded(
      proxy_info.has_value() && !proxy_info->is_direct());
}

}  // namespace

TorProfileServiceImpl::TorProfileServiceImpl(Profile* profile)
    : profile_(profile),
      weak_ptr_factory_(this) {
  // Return early since g_brave_browser_process and tor_client_updater are not
  // available in unit tests.
  if (profile_->AsTestingProfile()) {
    return;
  }

  g_brave_browser_process->tor_client_updater()->AddObserver(this);
  OnExecutableReady(GetTorExecutablePath());
}

TorProfileServiceImpl::~TorProfileServiceImpl() {
  if (tor_launcher_factory_)
    tor_launcher_factory_->RemoveObserver(this);
}

void TorProfileServiceImpl::OnExecutableReady(const base::FilePath& path) {
  if (path.empty())
    return;

  g_brave_browser_process->tor_client_updater()->RemoveObserver(this);

  tor_launcher_factory_ = TorLauncherFactory::GetInstance();
  tor_launcher_factory_->AddObserver(this);

  if (tor_launcher_factory_->GetTorPid() < 0) {
    LaunchTor();
  }
}

void TorProfileServiceImpl::LaunchTor() {
  tor::TorConfig config(GetTorExecutablePath(), GetTorProxyURI());
  tor_launcher_factory_->LaunchTorProcess(config);
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
  auto* storage_partition =
      BrowserContext::GetStoragePartitionForSite(profile_, url, false);
  if (!storage_partition) {
    storage_partition =
        content::BrowserContext::GetDefaultStoragePartition(profile_);
  }
  auto proxy_lookup_client_ptr =
      TorProxyLookupClient::CreateTorProxyLookupClient(std::move(callback));
  storage_partition->GetNetworkContext()->LookUpProxyForURL(
      url, std::move(proxy_lookup_client_ptr));
}

void TorProfileServiceImpl::KillTor() {
  if (tor_launcher_factory_)
    tor_launcher_factory_->KillTorProcess();
}

void TorProfileServiceImpl::NotifyTorLauncherCrashed() {
  for (auto& observer : observers_)
    observer.OnTorLauncherCrashed();
}

void TorProfileServiceImpl::NotifyTorCrashed(int64_t pid) {
  for (auto& observer : observers_)
    observer.OnTorCrashed(pid);
}

void TorProfileServiceImpl::NotifyTorLaunched(bool result, int64_t pid) {
  for (auto& observer : observers_)
    observer.OnTorLaunched(result, pid);
}

std::unique_ptr<net::ProxyConfigService>
TorProfileServiceImpl::CreateProxyConfigService() {
  proxy_config_service_ = new net::ProxyConfigServiceTor(GetTorProxyURI());
  return std::unique_ptr<net::ProxyConfigServiceTor>(proxy_config_service_);
}

}  // namespace tor
