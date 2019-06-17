/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_profile_service_impl.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/task/post_task.h"
#include "brave/browser/tor/tor_launcher_service_observer.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/network/public/mojom/network_context.mojom.h"

using content::BrowserContext;
using content::BrowserThread;

namespace tor {

TorProfileServiceImpl::TorProfileServiceImpl(Profile* profile)
    : profile_(profile), binding_(this) {
  tor_launcher_factory_ = TorLauncherFactory::GetInstance();
  tor_launcher_factory_->AddObserver(this);
}

TorProfileServiceImpl::~TorProfileServiceImpl() {
  tor_launcher_factory_->RemoveObserver(this);
}

void TorProfileServiceImpl::Shutdown() {
  TorProfileService::Shutdown();
}

void TorProfileServiceImpl::LaunchTor(const TorConfig& config) {
  tor_launcher_factory_->LaunchTorProcess(config);
}

void TorProfileServiceImpl::ReLaunchTor(const TorConfig& config) {
  tor_launcher_factory_->ReLaunchTorProcess(config);
}

void TorProfileServiceImpl::OnSetNewTorCircuitComplete(bool success) {
  if (tor_circuit_callback_)
    std::move(tor_circuit_callback_).Run(success);
}

void TorProfileServiceImpl::OnProxyLookupComplete(
    int32_t net_error,
    const base::Optional<net::ProxyInfo>& proxy_info) {
  bool success = proxy_info.has_value() && !proxy_info->is_direct();
  if (tor_circuit_callback_) {
    std::move(tor_circuit_callback_).Run(success);
    binding_.Close();
  }
}

void TorProfileServiceImpl::SetNewTorCircuit(const GURL& request_url,
                                             NewTorCircuitCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto* storage_partition =
    BrowserContext::GetStoragePartitionForSite(profile_, request_url, false);

  GURL::Replacements replacements;
  replacements.SetRef("NewTorCircuit",
                      url::Component(0, strlen("NewTorCircuit")));
  GURL url = request_url.ReplaceComponents(replacements);
  tor_circuit_callback_ = std::move(callback);

  network::mojom::ProxyLookupClientPtr proxy_lookup_client_ptr;
  binding_.Bind(
      mojo::MakeRequest(&proxy_lookup_client_ptr),
      base::CreateSingleThreadTaskRunnerWithTraits(
          {content::BrowserThread::UI, content::BrowserTaskType::kPreconnect}));
  binding_.set_connection_error_handler(
      base::BindOnce(&TorProfileServiceImpl::OnProxyLookupComplete,
                     base::Unretained(this), net::ERR_ABORTED, base::nullopt));
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
