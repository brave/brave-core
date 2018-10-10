/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/mock_tor_profile_service_impl.h"

#include "brave/browser/tor/tor_proxy_config_service.h"
#include "brave/common/tor/tor_test_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/site_instance.h"

using content::BrowserThread;
using content::SiteInstance;
using tor::TorProxyConfigService;

namespace tor {

MockTorProfileServiceImpl::MockTorProfileServiceImpl(Profile* profile) :
    profile_(profile) {
  base::FilePath path(kTestTorPath);
  std::string proxy(kTestTorProxy);
  config_ = TorConfig(path, proxy);
}

MockTorProfileServiceImpl::~MockTorProfileServiceImpl() {}

void MockTorProfileServiceImpl::LaunchTor(const TorConfig& config) {}

void MockTorProfileServiceImpl::ReLaunchTor(const TorConfig& config) {}


void MockTorProfileServiceImpl::SetNewTorCircuit(const GURL& request_url,
                                             const base::Closure& callback) {}

const TorConfig& MockTorProfileServiceImpl::GetTorConfig() {
  return config_;
}

int64_t MockTorProfileServiceImpl::GetTorPid() { return -1; }

void MockTorProfileServiceImpl::SetProxy(
    net::ProxyResolutionService* service, const GURL& request_url,
    bool new_circuit) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  GURL url = SiteInstance::GetSiteForURL(profile_, request_url);
  if (url.host().empty() || config_.empty())
    return;
  TorProxyConfigService::TorSetProxy(service, config_.proxy_string(),
                                     url.host(), nullptr, new_circuit);
}

}  // namespace tor
