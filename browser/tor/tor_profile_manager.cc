/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_profile_manager.h"

#include <algorithm>
#include <utility>

#include "base/feature_list.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/tor/tor_constants.h"
#include "brave/components/tor/tor_launcher_factory.h"
#include "brave/components/tor/tor_launcher_observer.h"
#include "brave/components/tor/tor_profile_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/safe_browsing/core/common/safe_browsing_prefs.h"
#include "components/translate/core/browser/translate_pref_names.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents_observer.h"
#include "net/base/features.h"
#include "third_party/blink/public/common/peerconnection/webrtc_ip_handling_policy.h"

namespace {

class TorBrowserListObserver : public BrowserListObserver {
 public:
  TorBrowserListObserver() {}
  ~TorBrowserListObserver() override {}

  size_t GetTorBrowserCount() {
    BrowserList* list = BrowserList::GetInstance();
    return std::count_if(list->begin(), list->end(), [](Browser* browser) {
      return browser->profile()->IsTor();
    });
  }

  // BrowserListObserver:
  void OnBrowserRemoved(Browser* browser) override {
    if (!browser || !browser->profile()->IsTor()) {
      return;
    }

    if (!GetTorBrowserCount()) {
      tor::TorProfileService* service =
          TorProfileServiceFactory::GetForContext(browser->profile());
      service->KillTor();
    }
  }
};

}  // namespace

// static
TorProfileManager& TorProfileManager::GetInstance() {
  static base::NoDestructor<TorProfileManager> instance;
  return *instance;
}

class TorTabNavigator final : public content::WebContentsObserver,
                              public TorLauncherObserver {
 public:
  static void Navigate(Browser* tor_browser, const GURL& url) {
    auto* tab = FindNTPTab(tor_browser);
    if (!tab) {
      tab = &chrome::NewTab(tor_browser);
    }
    if (!url.is_valid() || !tab) {
      return;
    }
    if (TorLauncherFactory::GetInstance()->IsTorConnected()) {
      // If Tor is connected just navigate to specified url.
      OpenURL(tab, url);
    } else {
      // Wait for the NTP to load and then go to the specified URL.
      // TorNavigationThrottle defers navigation until Tor is connected, so the
      // user can see the connection status.
      new TorTabNavigator(tab, url);
    }
  }

 private:
  TorTabNavigator(content::WebContents* web_contents, const GURL& url)
      : content::WebContentsObserver(web_contents), url_(url) {
    TorLauncherFactory::GetInstance()->AddObserver(this);
  }

  ~TorTabNavigator() final {
    TorLauncherFactory::GetInstance()->RemoveObserver(this);
  }

  void WebContentsDestroyed() final {
    Observe(nullptr);
    delete this;
  }

  void DidStopLoading() final {
    if (!url_.is_valid()) {
      return;
    }
    content::NavigationController::LoadURLParams params(url_);
    params.transition_type = ui::PAGE_TRANSITION_TYPED;
    web_contents()->GetController().LoadURLWithParams(params);
    if (web_contents()->GetDelegate()) {
      web_contents()->GetDelegate()->NavigationStateChanged(
          web_contents(), content::INVALIDATE_TYPE_URL);
    }
    url_ = GURL();
  }

  // TorLauncherObserver:
  void OnTorCircuitEstablished(bool result) final {
    if (!result) {
      return;
    }
    if (url_.is_valid()) {
      // Tor connected before the NTP is loaded.
      DidStopLoading();
    }
    if (web_contents()->GetDelegate()) {
      web_contents()->GetDelegate()->NavigationStateChanged(
          web_contents(), content::INVALIDATE_TYPE_ALL);
    }
    WebContentsDestroyed();
  }

  static void OpenURL(content::WebContents* web_contents, const GURL& url) {
    content::NavigationController::LoadURLParams params(url);
    params.transition_type = ui::PAGE_TRANSITION_TYPED;
    web_contents->GetController().LoadURLWithParams(params);
    if (web_contents->GetDelegate()) {
      web_contents->GetDelegate()->NavigationStateChanged(
          web_contents, content::INVALIDATE_TYPE_URL);
    }
  }

  static content::WebContents* FindNTPTab(Browser* tor_browser) {
    for (int i = 0; i < tor_browser->tab_strip_model()->count(); ++i) {
      auto* tab = tor_browser->tab_strip_model()->GetWebContentsAt(i);
      if (tab->GetURL() == tor_browser->GetNewTabURL()) {
        return tab;
      }
    }
    return nullptr;
  }

  GURL url_;
};

// static
Browser* TorProfileManager::SwitchToTorProfile(Profile* original_profile,
                                               const GURL& url) {
  Profile* tor_profile =
      TorProfileManager::GetInstance().GetTorProfile(original_profile);
  if (!tor_profile) {
    return nullptr;
  }

  // Find an existing Tor Browser, making a new one if no such Browser is
  // located.
  Browser* browser = chrome::FindTabbedBrowser(tor_profile, false);
  if (!browser && Browser::GetCreationStatusForProfile(tor_profile) ==
                      Browser::CreationStatus::kOk) {
    browser = Browser::Create(Browser::CreateParams(tor_profile, true));
  }
  if (browser) {
    TorTabNavigator::Navigate(browser, url);
    browser->window()->Activate();
    browser->window()->Show();
  }
  return browser;
}

// static
void TorProfileManager::CloseTorProfileWindows(Profile* tor_profile) {
  DCHECK(tor_profile);
  BrowserList::CloseAllBrowsersWithIncognitoProfile(
      tor_profile, base::DoNothing(), base::DoNothing(),
      true /* skip_beforeunload */);
}

TorProfileManager::TorProfileManager()
    : browser_list_observer_(new TorBrowserListObserver()) {
  BrowserList::AddObserver(browser_list_observer_.get());
}

TorProfileManager::~TorProfileManager() {
  BrowserList::RemoveObserver(browser_list_observer_.get());
}

Profile* TorProfileManager::GetTorProfile(Profile* profile) {
  if (TorProfileServiceFactory::IsTorDisabled(profile)) {
    return nullptr;
  }

  Profile* tor_profile = profile->GetOriginalProfile()->GetOffTheRecordProfile(
      Profile::OTRProfileID(tor::kTorProfileID),
      /*create_if_needed=*/true);

  const std::string context_id = tor_profile->UniqueId();
  auto it = tor_profiles_.find(context_id);
  if (it != tor_profiles_.end()) {
    return it->second;
  }

  InitTorProfileUserPrefs(tor_profile);

  tor_profile->AddObserver(this);
  tor_profiles_[context_id] = tor_profile;

  tor::TorProfileService* service =
      TorProfileServiceFactory::GetForContext(tor_profile);
  DCHECK(service);
  // TorLauncherFactory relies on OnExecutableReady to launch tor process so
  // we need to make sure tor binary is there every time
  service->RegisterTorClientUpdater();

  return tor_profile;
}

void TorProfileManager::CloseAllTorWindows() {
  for (const auto& it : tor_profiles_) {
    CloseTorProfileWindows(it.second);
  }
}

void TorProfileManager::OnProfileWillBeDestroyed(Profile* profile) {
  const std::string context_id = profile->UniqueId();
  tor_profiles_.erase(context_id);
  profile->RemoveObserver(this);
}

void TorProfileManager::InitTorProfileUserPrefs(Profile* profile) {
  PrefService* pref_service = profile->GetPrefs();
  pref_service->SetString(prefs::kWebRTCIPHandlingPolicy,
                          blink::kWebRTCIPHandlingDisableNonProxiedUdp);
  pref_service->SetBoolean(prefs::kSafeBrowsingEnabled, false);
  if (base::FeatureList::IsEnabled(net::features::kBraveTorWindowsHttpsOnly)) {
    pref_service->SetBoolean(prefs::kHttpsOnlyModeEnabled, true);
  }
  // https://blog.torproject.org/bittorrent-over-tor-isnt-good-idea
  pref_service->SetBoolean(kWebTorrentEnabled, false);

  // Disable the automatic translate bubble in Tor because we currently don't
  // support extensions in Tor mode and users cannot disable this through
  // settings page for Tor windows.
  pref_service->SetBoolean(translate::prefs::kOfferTranslateEnabled, false);
}
