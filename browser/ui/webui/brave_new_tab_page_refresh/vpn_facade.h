// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_VPN_FACADE_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_VPN_FACADE_H_

#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"

namespace content {
class WebContents;
}

#if BUILDFLAG(ENABLE_BRAVE_VPN)
namespace brave_vpn {
class BraveVpnService;
}

class BraveVPNController;
#endif

namespace brave_new_tab_page_refresh {

#if BUILDFLAG(ENABLE_BRAVE_VPN)

// Provides a simplified interface for accessing the Brave VPN service API from
// the new tab page. This adapter is primarily used to avoid preprocessor
// branching in `NewTabPageHandler`.
class VPNFacade {
 public:
  VPNFacade(content::WebContents& web_contents,
            brave_vpn::BraveVpnService* vpn_service);
  ~VPNFacade();

  VPNFacade(const VPNFacade&) = delete;
  VPNFacade& operator=(const VPNFacade&) = delete;

  void ReloadPurchasedState();
  void OpenPanel();
  void OpenAccountPage(brave_vpn::mojom::ManageURLType url_type);
  void RecordWidgetUsage();
  std::optional<std::string> GetWidgetPrefName();

 private:
  BraveVPNController* GetBraveVPNController();

  raw_ref<content::WebContents> web_contents_;
  raw_ptr<brave_vpn::BraveVpnService> vpn_service_;
};

#else

// Provides a no-op implementation for when the Brave VPN API does not exist.
class VPNFacade {
 public:
  VPNFacade() = default;
  ~VPNFacade() = default;

  VPNFacade(const VPNFacade&) = delete;
  VPNFacade& operator=(const VPNFacade&) = delete;

  void ReloadPurchasedState() {}
  void OpenPanel() {}
  void OpenAccountPage(brave_vpn::mojom::ManageURLType url_type) {}
  void RecordWidgetUsage() {}
  std::optional<std::string> GetWidgetPrefName() { return std::nullopt; }
};

#endif

}  // namespace brave_new_tab_page_refresh

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_VPN_FACADE_H_
