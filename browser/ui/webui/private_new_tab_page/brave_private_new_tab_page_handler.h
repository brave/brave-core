// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_PRIVATE_NEW_TAB_PAGE_BRAVE_PRIVATE_NEW_TAB_PAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_PRIVATE_NEW_TAB_PAGE_BRAVE_PRIVATE_NEW_TAB_PAGE_HANDLER_H_

#include <stdint.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/scoped_observation.h"
#include "base/timer/timer.h"
#include "brave/components/brave_private_new_tab_ui/common/brave_private_new_tab.mojom.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "brave/components/tor/tor_launcher_observer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/browser/brave_vpn_service_observer.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#endif

class BravePrivateNewTabPageVpnBrowserTest;
class Profile;

namespace content {
class WebContents;
}  // namespace content

#if BUILDFLAG(ENABLE_TOR)
class TorLauncherFactory;
#endif

class BravePrivateNewTabPageHandler
    : public brave_private_new_tab::mojom::PageHandler,
#if BUILDFLAG(ENABLE_BRAVE_VPN)
      public brave_vpn::BraveVpnServiceObserver,
#endif
      public TorLauncherObserver {
 public:
  // Synchronous reads of what the page needs for its first paint, passed as
  // load time data so it can render without waiting on mojo round trips.
  static bool IsDisclaimerDismissed(Profile* profile);
  // Whether a Tor window can't be opened from `profile`: Tor disabled by the
  // user or policy, guest profiles, and incognito disabled or forced.
  static bool IsTorDisabled(Profile* profile);
  static bool IsTorConnected();
  // Whether to show the VPN promo, from the live purchase state when it has
  // loaded or else the answer cached by an earlier page. std::nullopt when
  // neither is known, in which case the page waits on GetShouldShowVpnPromo.
  static std::optional<bool> GetInitialShouldShowVpnPromo(Profile* profile);

  BravePrivateNewTabPageHandler(
      Profile* profile,
      content::WebContents* web_contents,
      mojo::PendingReceiver<brave_private_new_tab::mojom::PageHandler>
          receiver);

  BravePrivateNewTabPageHandler(const BravePrivateNewTabPageHandler&) = delete;
  BravePrivateNewTabPageHandler& operator=(
      const BravePrivateNewTabPageHandler&) = delete;

  ~BravePrivateNewTabPageHandler() override;

  // brave_private_new_tab::mojom::PageHandler overrides:
  void SetClientPage(
      mojo::PendingRemote<brave_private_new_tab::mojom::PrivateTabPage> page)
      override;
  void SetDisclaimerDismissed(bool dismissed) override;
  void GetIsTorConnected(GetIsTorConnectedCallback callback) override;
  void GoToBraveSearch(const std::string& input, bool open_new_tab) override;
  void GoToBraveSupport() override;
  void GetShouldShowVpnPromo(GetShouldShowVpnPromoCallback callback) override;
  void OpenVpnCheckout() override;
  void OpenTorWindow() override;

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  // brave_vpn::BraveVpnServiceObserver overrides:
  void OnPurchasedStateChanged(
      brave_vpn::mojom::PurchasedState state,
      const std::optional<std::string>& description) override;
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)

 private:
  friend class ::BravePrivateNewTabPageVpnBrowserTest;

  // Replies to the pending GetShouldShowVpnPromo callbacks, and caches `show`
  // for later pages when `cache` is set.
  void ResolveVpnPromo(bool show, bool cache);
  void OnVpnPromoTimeout();

  // TorLauncherObserver overrides:
  void OnTorCircuitEstablished(bool result) override;
  void OnTorInitializing(const std::string& percentage,
                         const std::string& message) override;

  void OnTorCircuitTimer(brave_private_new_tab::mojom::ConnectionStatus status);

  // Handle back to the page by which we can pass results.
  mojo::Remote<brave_private_new_tab::mojom::PrivateTabPage> page_;

  // The Profile* handed to us in our constructor.
  raw_ptr<Profile> profile_;

  raw_ptr<content::WebContents> web_contents_ = nullptr;

#if BUILDFLAG(ENABLE_TOR)
  raw_ptr<TorLauncherFactory> tor_launcher_factory_ = nullptr;
#endif
  // Timer for detecting a Tor connection failuer. It starts at each connection
  // event and shots when the time between events exceeds kStuckPeriod seconds.
  base::OneShotTimer stuck_timer_;

  // GetShouldShowVpnPromo replies waiting for the VPN purchase state to load.
  std::vector<GetShouldShowVpnPromoCallback> vpn_promo_callbacks_;
  // Bounds that wait, so the page's skeleton card can't outlive a load that
  // never settles.
  base::OneShotTimer vpn_promo_timeout_;
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  bool is_observing_vpn_ = false;
#endif

  mojo::Receiver<brave_private_new_tab::mojom::PageHandler> receiver_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_PRIVATE_NEW_TAB_PAGE_BRAVE_PRIVATE_NEW_TAB_PAGE_HANDLER_H_
