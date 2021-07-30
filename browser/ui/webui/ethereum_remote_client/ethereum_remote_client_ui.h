/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_ETHEREUM_REMOTE_CLIENT_ETHEREUM_REMOTE_CLIENT_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_ETHEREUM_REMOTE_CLIENT_ETHEREUM_REMOTE_CLIENT_UI_H_

#include <string>

#include "content/public/browser/web_ui_controller.h"

class EthereumRemoteClientUI : public content::WebUIController {
 public:
  EthereumRemoteClientUI(content::WebUI* web_ui, const std::string& host);
  ~EthereumRemoteClientUI() override;
  EthereumRemoteClientUI(const EthereumRemoteClientUI&) = delete;
  EthereumRemoteClientUI& operator=(const EthereumRemoteClientUI&) = delete;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_ETHEREUM_REMOTE_CLIENT_ETHEREUM_REMOTE_CLIENT_UI_H_
