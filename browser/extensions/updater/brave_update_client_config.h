// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_EXTENSIONS_UPDATER_BRAVE_UPDATE_CLIENT_CONFIG_H_
#define BRAVE_BROWSER_EXTENSIONS_UPDATER_BRAVE_UPDATE_CLIENT_CONFIG_H_

#include <optional>
#include <string>

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "chrome/browser/extensions/updater/chrome_update_client_config.h"

class GURL;

namespace content {
class BrowserContext;
}

namespace update_client {
class NetworkFetcherFactory;
}

namespace extensions {

class ExtensionUpdateClientBaseTest;

class BraveUpdateClientConfig : public ChromeUpdateClientConfig {
 public:
  BraveUpdateClientConfig(const BraveUpdateClientConfig&) = delete;
  BraveUpdateClientConfig& operator=(const BraveUpdateClientConfig&) = delete;

  static scoped_refptr<ChromeUpdateClientConfig> Create(
      content::BrowserContext* context,
      std::optional<GURL> url_override);

  using ChromeUpdateClientConfig::ChromeUpdateClientConfig;

  std::string GetChannel() const override;
  scoped_refptr<update_client::NetworkFetcherFactory> GetNetworkFetcherFactory()
      override;

 protected:
  friend class base::RefCountedThreadSafe<BraveUpdateClientConfig>;
  friend class ExtensionUpdateClientBaseTest;

  ~BraveUpdateClientConfig() override;
};

}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_UPDATER_BRAVE_UPDATE_CLIENT_CONFIG_H_
