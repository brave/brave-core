/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/web_view/internal/app/application_context.h"

#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "components/component_updater/component_updater_service.h"
#import "components/os_crypt/async/browser/os_crypt_async.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/components/security_interstitials/safe_browsing/safe_browsing_service_impl.h"
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"
#include "net/log/net_log.h"
#include "services/network/network_change_manager.h"
#include "services/network/public/cpp/network_connection_tracker.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"

namespace ios_web_view {

ApplicationContext* ApplicationContext::GetInstance() {
  static base::NoDestructor<ios_web_view::ApplicationContext> instance;
  return instance.get();
}

ApplicationContext::ApplicationContext() = default;

ApplicationContext::~ApplicationContext() = default;

PrefService* ApplicationContext::GetLocalState() {
  return GetApplicationContext()->GetLocalState();
}

net::URLRequestContextGetter* ApplicationContext::GetSystemURLRequestContext() {
  return GetApplicationContext()->GetSystemURLRequestContext();
}

scoped_refptr<network::SharedURLLoaderFactory>
ApplicationContext::GetSharedURLLoaderFactory() {
  return GetApplicationContext()->GetSharedURLLoaderFactory();
}
network::mojom::NetworkContext* ApplicationContext::GetSystemNetworkContext() {
  return GetApplicationContext()->GetSystemNetworkContext();
}

network::NetworkConnectionTracker*
ApplicationContext::GetNetworkConnectionTracker() {
  return GetApplicationContext()->GetNetworkConnectionTracker();
}

const std::string& ApplicationContext::GetApplicationLocale() {
  return GetApplicationContext()->GetApplicationLocale();
}

net::NetLog* ApplicationContext::GetNetLog() {
  return GetApplicationContext()->GetNetLog();
}

component_updater::ComponentUpdateService*
ApplicationContext::GetComponentUpdateService() {
  return GetApplicationContext()->GetComponentUpdateService();
}

os_crypt_async::OSCryptAsync* ApplicationContext::GetOSCryptAsync() {
  return GetApplicationContext()->GetOSCryptAsync();
}

void ApplicationContext::PreCreateThreads() {}

void ApplicationContext::PostCreateThreads() {}

void ApplicationContext::SaveState() {}

void ApplicationContext::PostDestroyThreads() {}

SafeBrowsingService* ApplicationContext::GetSafeBrowsingService() {
  return GetApplicationContext()->GetSafeBrowsingService();
}

void ApplicationContext::ShutdownSafeBrowsingServiceIfNecessary() {}

}  // namespace ios_web_view
