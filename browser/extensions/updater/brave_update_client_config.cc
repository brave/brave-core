// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/extensions/updater/brave_update_client_config.h"

#include <optional>
#include <string>

#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/no_destructor.h"
#include "components/update_client/net/network_chromium.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "url/gurl.h"


namespace extensions {

namespace {

using FactoryCallback = ChromeUpdateClientConfig::FactoryCallback;

// static
static FactoryCallback& GetFactoryCallback() {
  static base::NoDestructor<FactoryCallback> factory;
  return *factory;
}

}  // namespace

std::string BraveUpdateClientConfig::GetChannel() const {
  return std::string("stable");
}

scoped_refptr<update_client::NetworkFetcherFactory>
BraveUpdateClientConfig::GetNetworkFetcherFactory() {
  if (!network_fetcher_factory_) {
    network_fetcher_factory_ =
        base::MakeRefCounted<update_client::NetworkFetcherChromiumFactory>(
            context_->GetDefaultStoragePartition()
                ->GetURLLoaderFactoryForBrowserProcess(),
            // Unlike ChromeUpdateClientConfig, which allows to send cookies for
            // chrome.google.com, we won't be sending any cookies.
            base::BindRepeating([](const GURL& url) { return false; }));
  }
  return network_fetcher_factory_;
}

BraveUpdateClientConfig::~BraveUpdateClientConfig() = default;

// static
scoped_refptr<ChromeUpdateClientConfig> BraveUpdateClientConfig::Create(
    content::BrowserContext* context,
    std::optional<GURL> update_url_override) {
  FactoryCallback& factory = GetFactoryCallback();
  return factory.is_null() ? base::MakeRefCounted<BraveUpdateClientConfig>(
                                 context, update_url_override)
                           : factory.Run(context);
}

}  // namespace extensions
