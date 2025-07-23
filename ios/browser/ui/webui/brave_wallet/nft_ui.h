// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WALLET_NFT_UI_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WALLET_NFT_UI_H_

#include "ios/web/public/webui/web_ui_ios.h"
#include "ios/web/public/webui/web_ui_ios_controller.h"

namespace nft {

class UntrustedNftUI : public web::WebUIIOSController {
 public:
  explicit UntrustedNftUI(web::WebUIIOS* web_ui, const GURL& url);
  UntrustedNftUI(const UntrustedNftUI&) = delete;
  UntrustedNftUI& operator=(const UntrustedNftUI&) = delete;
  ~UntrustedNftUI() override;
};

}  // namespace nft

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WALLET_NFT_UI_H_
