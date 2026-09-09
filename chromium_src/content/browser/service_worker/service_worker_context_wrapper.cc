/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/containers/flat_set.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_ui_url_loader_factory.h"
#include "content/public/browser/webui_config.h"
#include "content/public/browser/webui_config_map.h"
#include "content/public/common/url_constants.h"
#include "third_party/blink/public/common/loader/url_loader_factory_bundle.h"
#include "url/gurl.h"

namespace content {

namespace {

// Gives |bundle_info| a factory that can serve |scope|'s WebUI resources, so
// that the script of a chrome-untrusted:// service worker can be fetched.
//
// The fetch is browser-initiated, so it reaches neither the frame nor the
// worker ContentBrowserClient factory hooks, and it happens without a
// WebUIController - which is what would ordinarily register the data source
// the script has to come from. See WebUIConfig::RegisterURLDataSource().
void MaybeAddUntrustedWebUIScriptFactory(
    BrowserContext* browser_context,
    const GURL& scope,
    network::PendingSharedURLLoaderFactory* bundle_info) {
  if (!scope.SchemeIs(kChromeUIUntrustedScheme)) {
    return;
  }

  // Fail closed for a scope with no WebUI behind it: without a config there is
  // no data source to register, and nothing legitimate to serve.
  WebUIConfig* config =
      WebUIConfigMap::GetInstance().GetConfig(browser_context, scope);
  if (!config) {
    return;
  }
  config->RegisterURLDataSource(browser_context);

  static_cast<blink::PendingURLLoaderFactoryBundle*>(bundle_info)
      ->pending_scheme_specific_factories()
      .emplace(kChromeUIUntrustedScheme,
               CreateWebUIURLLoaderFactoryForWorker(
                   browser_context, kChromeUIUntrustedScheme,
                   /*allowed_hosts=*/base::flat_set<std::string>()));
}

}  // namespace

}  // namespace content

#include <content/browser/service_worker/service_worker_context_wrapper.cc>
