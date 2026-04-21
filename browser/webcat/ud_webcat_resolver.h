/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WEBCAT_UD_WEBCAT_RESOLVER_H_
#define BRAVE_BROWSER_WEBCAT_UD_WEBCAT_RESOLVER_H_

#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/webcat/core/webcat_resolver.h"

namespace brave_wallet {

class JsonRpcService;

}  // namespace brave_wallet

namespace webcat {

class UdWebcatResolver : public WebcatResolver {
 public:
  explicit UdWebcatResolver(brave_wallet::JsonRpcService* json_rpc_service);
  ~UdWebcatResolver() override;

  UdWebcatResolver(const UdWebcatResolver&) = delete;
  UdWebcatResolver& operator=(const UdWebcatResolver&) = delete;

  void Resolve(const std::string& domain,
               ResolveCallback callback) override;

 private:
  void OnGetWebcatCid(ResolveCallback callback,
                      const std::string& cid,
                      brave_wallet::mojom::ProviderError error,
                      const std::string& error_message);

  raw_ptr<brave_wallet::JsonRpcService> json_rpc_service_;
};

}  // namespace webcat

#endif  // BRAVE_BROWSER_WEBCAT_UD_WEBCAT_RESOLVER_H_
