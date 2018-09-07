/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_profile_service_impl.h"

#include "base/bind.h"
#include "brave/browser/tor/tor_launcher_service_observer.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

using content::BrowserContext;
using content::BrowserThread;
using content::SiteInstance;

namespace tor {

TorProfileServiceImpl::TorProfileServiceImpl(Profile* profile) :
    profile_(profile) {
  //TODO: remove it
  LOG(ERROR) << profile_;
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

void TorProfileServiceImpl::SetNewTorCircuitOnIOThread(
    const scoped_refptr<net::URLRequestContextGetter>& getter,
    const std::string& host) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  const TorConfig tor_config = tor_launcher_factory_->GetTorConfig();
  if (tor_config.empty())
    return;
  auto* proxy_resolution_service =
    getter->GetURLRequestContext()->proxy_resolution_service();
  DCHECK(proxy_resolution_service);
  TorProxyConfigService::TorSetProxy(proxy_resolution_service,
                                     tor_config.proxy_string(),
                                     host,
                                     &tor_proxy_map_,
                                     true);
}


void TorProfileServiceImpl::SetNewTorCircuit(const GURL& request_url,
                                             const base::Closure& callback) {
  GURL url = SiteInstance::GetSiteForURL(profile_, request_url);
  if (url.host().empty())
    return;
  auto* storage_partition =
    BrowserContext::GetStoragePartitionForSite(profile_, url , false);

  net::URLRequestContextGetter* url_request_context_getter =
    storage_partition->GetURLRequestContext();
  DCHECK(url_request_context_getter);

  BrowserThread::PostTaskAndReply(
    BrowserThread::IO, FROM_HERE,
    base::Bind(&TorProfileServiceImpl::SetNewTorCircuitOnIOThread,
               base::Unretained(this),
               base::WrapRefCounted(url_request_context_getter),
               url.host()),
    callback);

}

const TorConfig& TorProfileServiceImpl::GetTorConfig() {
  return tor_launcher_factory_->GetTorConfig();
}

int64_t TorProfileServiceImpl::GetTorPid() {
  return tor_launcher_factory_->GetTorPid();
}

void TorProfileServiceImpl::SetProxy(net::ProxyResolutionService* service,
                                     const GURL& request_url,bool new_circuit) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  const TorConfig tor_config = tor_launcher_factory_->GetTorConfig();
  GURL url = SiteInstance::GetSiteForURL(profile_, request_url);
  if (url.host().empty() || tor_config.empty())
    return;
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::Bind(&TorProxyConfigService::TorSetProxy,
                                     service,
                                     tor_config.proxy_string(),
                                     url.host(),
                                     &tor_proxy_map_,
                                     new_circuit));
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
