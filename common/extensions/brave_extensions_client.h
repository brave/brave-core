// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMMON_EXTENSIONS_BRAVE_EXTENSIONS_CLIENT_H_
#define BRAVE_COMMON_EXTENSIONS_BRAVE_EXTENSIONS_CLIENT_H_

#include "chrome/common/extensions/chrome_extensions_client.h"

namespace extensions {

class BraveExtensionsClient : public ChromeExtensionsClient {
 public:
  BraveExtensionsClient();
  BraveExtensionsClient(const BraveExtensionsClient&) = delete;
  BraveExtensionsClient& operator=(const BraveExtensionsClient&) = delete;

  void InitializeWebStoreUrls(base::CommandLine* command_line) override;
  const GURL& GetWebstoreUpdateURL() const override;

 private:
  GURL webstore_update_url_;
};

}  // namespace extensions

#endif  // BRAVE_COMMON_EXTENSIONS_BRAVE_EXTENSIONS_CLIENT_H_
