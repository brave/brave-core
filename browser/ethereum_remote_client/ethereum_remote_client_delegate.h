/* Copyright (c) 2021 The Brave Authors. All rights reserved.
+ * This Source Code Form is subject to the terms of the Mozilla Public
+ * License, v. 2.0. If a copy of the MPL was not distributed with this file,
+ * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_ETHEREUM_REMOTE_CLIENT_ETHEREUM_REMOTE_CLIENT_DELEGATE_H_
#define BRAVE_BROWSER_ETHEREUM_REMOTE_CLIENT_ETHEREUM_REMOTE_CLIENT_DELEGATE_H_

namespace content {
class BrowserContext;
}  // namespace content

class EthereumRemoteClientDelegate {
 public:
  virtual ~EthereumRemoteClientDelegate() = default;
  virtual void MaybeLoadCryptoWalletsExtension(
      content::BrowserContext* context) = 0;
};

#endif  // BRAVE_BROWSER_ETHEREUM_REMOTE_CLIENT_ETHEREUM_REMOTE_CLIENT_DELEGATE_H_
