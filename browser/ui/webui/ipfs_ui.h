/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_IPFS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_IPFS_UI_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/browser/ui/webui/basic_ui.h"
#include "content/public/browser/web_ui_message_handler.h"

class PrefChangeRegistrar;

class IPFSDOMHandler : public content::WebUIMessageHandler {
 public:
  IPFSDOMHandler();
  ~IPFSDOMHandler() override;

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  void HandleGetConnectedPeers(const base::ListValue* args);
  void OnGetConnectedPeers(bool success,
                           const std::vector<std::string>& peers);

  base::WeakPtrFactory<IPFSDOMHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(IPFSDOMHandler);
};

// The WebUI for brave://ipfs
class IPFSUI : public BasicUI {
 public:
  IPFSUI(content::WebUI* web_ui, const std::string& host);
  ~IPFSUI() override;

 private:
  // BasicUI overrides:
  void UpdateWebUIProperties() override;

  void CustomizeWebUIProperties(content::RenderFrameHost* render_frame_host);
  void OnPreferenceChanged();

  std::unique_ptr<PrefChangeRegistrar> pref_change_registrar_;

  DISALLOW_COPY_AND_ASSIGN(IPFSUI);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_IPFS_UI_H_
