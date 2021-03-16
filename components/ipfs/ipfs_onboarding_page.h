/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPFS_ONBOARDING_PAGE_H_
#define BRAVE_COMPONENTS_IPFS_IPFS_ONBOARDING_PAGE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "base/values.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/ipfs_service_observer.h"
#include "components/security_interstitials/content/security_interstitial_page.h"
#include "ui/native_theme/native_theme.h"
#include "ui/native_theme/native_theme_observer.h"
#include "url/gurl.h"

namespace content {
class WebContents;
}  // namespace content

namespace security_interstitials {
class SecurityInterstitialControllerClient;
}  // namespace security_interstitials

namespace ipfs {

class IpfsService;

// IPFSOnboardingPage is the interstitial page which will be shown when the
// browser tries to access IPFS contents if ASK mode is selected in settings.
class IPFSOnboardingPage
    : public security_interstitials::SecurityInterstitialPage,
      public ipfs::IpfsServiceObserver,
      public ui::NativeThemeObserver {
 public:
  // Interstitial type, used in tests.
  static const security_interstitials::SecurityInterstitialPage::TypeID
      kTypeForTesting;

  // Commands from a page which means whether we launch a local node
  // or we should redirect user to a public gateway.
  enum IPFSOnboardingCommandId {
    USE_LOCAL_NODE = 0,
    USE_PUBLIC_GATEWAY = 1,
    LEARN_MORE = 2,
    OPEN_SETTINGS = 3,
  };

  // Commands to send to the page to provide feedback to the user.
  enum IPFSOnboardingResponse {
    LOCAL_NODE_ERROR = 0,
    THEME_CHANGED = 1,
    LOCAL_NODE_LAUNCHED = 2,
    NO_PEERS_AVAILABLE = 3,
    NO_PEERS_LIMIT = 4,
    INSTALLATION_ERROR = 5
  };

  explicit IPFSOnboardingPage(
      IpfsService* ipfs_service,
      content::WebContents* web_contents,
      const GURL& request_url,
      std::unique_ptr<
          security_interstitials::SecurityInterstitialControllerClient>
          controller);
  ~IPFSOnboardingPage() override;

  IPFSOnboardingPage(const IPFSOnboardingPage&) = delete;
  IPFSOnboardingPage& operator=(const IPFSOnboardingPage&) = delete;

  // SecurityInterstitialPage::
  void OnInterstitialClosing() override {}
  void CommandReceived(const std::string& command) override;
  security_interstitials::SecurityInterstitialPage::TypeID GetTypeForTesting()
      override;
  void Proceed();

  // ipfs::IpfsServiceObserver
  void OnIpfsLaunched(bool result, int64_t pid) override;
  void OnInstallationEvent(ipfs::ComponentUpdaterEvents event) override;
  void OnIpfsShutdown() override;
  void OnGetConnectedPeers(bool succes,
                           const std::vector<std::string>& peers) override;

  // ui::NativeThemeObserver overrides:
  void OnNativeThemeUpdated(ui::NativeTheme* observed_theme) override;

 protected:
  // SecurityInterstitialPage::
  void PopulateInterstitialStrings(
      base::DictionaryValue* load_time_data) override;
  int GetHTMLTemplateId() override;

 private:
  friend class IpfsNavigationThrottleBrowserTest;

  std::string GetThemeType(ui::NativeTheme* theme) const;
  void SetupProtocolHandlers();
  void RespondToPage(IPFSOnboardingResponse command,
                     const std::u16string& text);
  void UseLocalNode();
  void UsePublicGateway();
  void GetConnectedPeers();
  void ReportDaemonStopped();
  bool IsLocalNodeMode();

  IpfsService* ipfs_service_ = nullptr;
  base::TimeTicks start_time_ticks_;
  base::ScopedObservation<ipfs::IpfsService, ipfs::IpfsServiceObserver>
      service_observer_{this};
  base::ScopedObservation<ui::NativeTheme, ui::NativeThemeObserver>
      theme_observer_{this};
  base::WeakPtrFactory<IPFSOnboardingPage> weak_ptr_factory_{this};
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IPFS_ONBOARDING_PAGE_H_
