/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_profile_service_impl.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/task/post_task.h"
#include "brave/browser/brave_browser_process_impl.h"
// TODO(bridiver) - move this out of extensions
#include "brave/browser/extensions/brave_tor_client_updater.h"
#include "brave/browser/tor/tor_launcher_service_observer.h"
#include "brave/common/tor/pref_names.h"
#include "brave/common/tor/tor_constants.h"
#include "brave/net/proxy_resolution/proxy_config_service_tor.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "net/url_request/url_request_context.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/proxy_lookup_client.mojom.h"

using content::BrowserContext;
using content::BrowserThread;

namespace tor {

namespace {

class TorProxyLookupClient : public network::mojom::ProxyLookupClient {
 public:
  static network::mojom::ProxyLookupClientPtr CreateTorProxyLookupClient(
      NewTorCircuitCallback callback) {
    auto* lookup_client = new TorProxyLookupClient(std::move(callback));
    return lookup_client->GetProxyLookupClientPtr();
  }

 private:
  TorProxyLookupClient(NewTorCircuitCallback callback)
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
            {content::BrowserThread::UI, content::BrowserTaskType::kPreconnect}));
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
    bool success = proxy_info.has_value() && !proxy_info->is_direct();
    std::move(callback_).Run(success);
    delete this;
  }

  NewTorCircuitCallback callback_;
  mojo::Binding<network::mojom::ProxyLookupClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(TorProxyLookupClient);
};

void OnProfileCreatedOnIOThread(Profile* profile, scoped_refptr<net::URLRequestContextGetter> request_context) {
  auto* url_request_context = request_context->GetURLRequestContext();
  auto* service = url_request_context->proxy_resolution_service();
  net::ProxyConfigServiceTor::SetTorProxyMap(service, profile);
}

void OnProfileDestroyedOnIOThread(
    Profile* profile,
    scoped_refptr<net::URLRequestContextGetter> getter) {
  auto* context = getter->GetURLRequestContext();
  auto* service = context->proxy_resolution_service();
  net::ProxyConfigServiceTor::UnsetTorProxyMap(service, profile);
}

void SetNewTorCircuitOnIOThread(Profile* profile,
                                GURL request_url) {
  net::ProxyConfigServiceTor::ResetTorProxyMap(profile, request_url);
}

}  // namespace

TorProfileServiceImpl::TorProfileServiceImpl(Profile* profile)
    : profile_(profile),
      weak_ptr_factory_(this) {
  tor_launcher_factory_ = TorLauncherFactory::GetInstance();
  tor_launcher_factory_->AddObserver(this);

  if (GetTorPid() < 0) {
    base::FilePath path =
        g_brave_browser_process->tor_client_updater()->GetExecutablePath();
    std::string proxy = g_browser_process->local_state()->GetString(
        tor::prefs::kTorProxyString);
    tor::TorConfig config(path, proxy);
    LaunchTor(config);
  }

  scoped_refptr<net::URLRequestContextGetter> getter(
      profile->GetRequestContext());
  getter->GetNetworkTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&OnProfileCreatedOnIOThread, profile, getter));
}

TorProfileServiceImpl::~TorProfileServiceImpl() {
  tor_launcher_factory_->RemoveObserver(this);
}

void TorProfileServiceImpl::Shutdown() {
  TorProfileService::Shutdown();

  scoped_refptr<net::URLRequestContextGetter> getter(
      profile_->GetRequestContext());
  getter->GetNetworkTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&OnProfileDestroyedOnIOThread, profile_, getter));
}

void TorProfileServiceImpl::LaunchTor(const TorConfig& config) {
  tor_launcher_factory_->LaunchTorProcess(config);
}

void TorProfileServiceImpl::ReLaunchTor(const TorConfig& config) {
  tor_launcher_factory_->ReLaunchTorProcess(config);
}

void TorProfileServiceImpl::SetNewTorCircuit(const GURL& request_url,
                                             NewTorCircuitCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  scoped_refptr<net::URLRequestContextGetter> getter(
      profile_->GetRequestContext());
  getter->GetNetworkTaskRunner()->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(&SetNewTorCircuitOnIOThread,
                     profile_,
                     request_url),
      base::BindOnce(&TorProfileServiceImpl::SetNewTorCircuitOnUIThread,
          weak_ptr_factory_.GetWeakPtr(),
          std::move(callback),
          request_url));
}

void TorProfileServiceImpl::SetNewTorCircuitOnUIThread(
    NewTorCircuitCallback callback, const GURL& url) {
  auto* storage_partition =
      BrowserContext::GetStoragePartitionForSite(profile_, url, false);

  // TorProxyLookupClient self deletes on proxy lookup completion
  auto proxy_lookup_client_ptr =
      TorProxyLookupClient::CreateTorProxyLookupClient(std::move(callback));

  // Force lookup to erase the old circuit
  storage_partition->GetNetworkContext()->LookUpProxyForURL(
      url, std::move(proxy_lookup_client_ptr));
}

const TorConfig& TorProfileServiceImpl::GetTorConfig() {
  return tor_launcher_factory_->GetTorConfig();
}

int64_t TorProfileServiceImpl::GetTorPid() {
  return tor_launcher_factory_->GetTorPid();
}

void TorProfileServiceImpl::KillTor() {
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


}  // namespace tor
