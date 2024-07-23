/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_APP_APPLICATION_CONTEXT_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_APP_APPLICATION_CONTEXT_H_

#include <memory>
#include <string>

#include "base/memory/scoped_refptr.h"
#include "base/no_destructor.h"
#include "base/sequence_checker.h"
#include "ios/web/public/init/network_context_owner.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/network/public/mojom/network_service.mojom.h"

namespace component_updater {
class ComponentUpdateService;
}

namespace net {
class NetLog;
class URLRequestContextGetter;
}  // namespace net

namespace network {
class NetworkChangeManager;
class NetworkConnectionTracker;
class SharedURLLoaderFactory;
class WeakWrapperSharedURLLoaderFactory;
namespace mojom {
class NetworkContext;
}
}  // namespace network

namespace os_crypt_async {
class OSCryptAsync;
}

class PrefService;
class SafeBrowsingService;

namespace ios_web_view {

class WebViewIOThread;

// Exposes application global state objects.
class ApplicationContext {
 public:
  static ApplicationContext* GetInstance();

  ApplicationContext(const ApplicationContext&) = delete;
  ApplicationContext& operator=(const ApplicationContext&) = delete;

  // Gets the preferences associated with this application.
  PrefService* GetLocalState();

  // Gets the URL request context associated with this application.
  net::URLRequestContextGetter* GetSystemURLRequestContext();

  scoped_refptr<network::SharedURLLoaderFactory> GetSharedURLLoaderFactory();
  network::mojom::NetworkContext* GetSystemNetworkContext();

  // Returns the NetworkConnectionTracker instance for this ApplicationContext.
  network::NetworkConnectionTracker* GetNetworkConnectionTracker();

  // Gets the locale used by the application.
  const std::string& GetApplicationLocale();

  // Gets the NetLog.
  net::NetLog* GetNetLog();

  // Gets the ComponentUpdateService.
  component_updater::ComponentUpdateService* GetComponentUpdateService();

  // Gets the application specific OSCryptAsync instance.
  os_crypt_async::OSCryptAsync* GetOSCryptAsync();

  // Creates state tied to application threads. It is expected this will be
  // called from web::WebMainParts::PreCreateThreads.
  void PreCreateThreads();

  // Called after the browser threads are created. It is expected this will be
  // called from web::WebMainParts::PostCreateThreads.
  void PostCreateThreads();

  // Saves application context state if |local_state_| exists. This should be
  // called during shutdown to save application state.
  void SaveState();

  // Destroys state tied to application threads. It is expected this will be
  // called from web::WebMainParts::PostDestroyThreads.
  void PostDestroyThreads();

  // Gets the SafeBrowsingService.
  SafeBrowsingService* GetSafeBrowsingService();

  // Shuts down SafeBrowsingService if it was created.
  void ShutdownSafeBrowsingServiceIfNecessary();

 private:
  friend class base::NoDestructor<ApplicationContext>;

  ApplicationContext();
  ~ApplicationContext();
};

}  // namespace ios_web_view

#endif
