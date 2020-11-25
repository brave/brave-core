// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at http://mozilla.org/MPL/2.0/.

// TODO(bridiver) - convert to sublcass chromium_src override
#include "ios/chrome/browser/signin/ios_chrome_signin_client.h"

#include "components/signin/ios/browser/account_consistency_service.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/signin/account_consistency_service_factory.h"
#include "ios/chrome/browser/signin/gaia_auth_fetcher_ios.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

IOSChromeSigninClient::IOSChromeSigninClient(
    ChromeBrowserState* browser_state,
    scoped_refptr<content_settings::CookieSettings> cookie_settings,
    scoped_refptr<HostContentSettingsMap> host_content_settings_map)
    : network_callback_helper_(
          std::make_unique<WaitForNetworkCallbackHelper>()),
      browser_state_(browser_state),
      cookie_settings_(cookie_settings),
      host_content_settings_map_(host_content_settings_map) {}

IOSChromeSigninClient::~IOSChromeSigninClient() {
}

void IOSChromeSigninClient::Shutdown() {
  network_callback_helper_.reset();
}

PrefService* IOSChromeSigninClient::GetPrefs() {
  return browser_state_->GetPrefs();
}

scoped_refptr<network::SharedURLLoaderFactory>
IOSChromeSigninClient::GetURLLoaderFactory() {
  return browser_state_->GetSharedURLLoaderFactory();
}

network::mojom::CookieManager* IOSChromeSigninClient::GetCookieManager() {
  return browser_state_->GetCookieManager();
}

void IOSChromeSigninClient::DoFinalInit() {}

bool IOSChromeSigninClient::AreSigninCookiesAllowed() {
  return false;
}

bool IOSChromeSigninClient::AreSigninCookiesDeletedOnExit() {
  return true;
}

void IOSChromeSigninClient::AddContentSettingsObserver(
    content_settings::Observer* observer) {}

void IOSChromeSigninClient::RemoveContentSettingsObserver(
    content_settings::Observer* observer) {}

void IOSChromeSigninClient::DelayNetworkCall(base::OnceClosure callback) {
  network_callback_helper_->HandleCallback(std::move(callback));
}

std::unique_ptr<GaiaAuthFetcher> IOSChromeSigninClient::CreateGaiaAuthFetcher(
    GaiaAuthConsumer* consumer,
    gaia::GaiaSource source) {
  return std::make_unique<GaiaAuthFetcherIOS>(
      consumer, source, GetURLLoaderFactory(), browser_state_);
}

void IOSChromeSigninClient::PreGaiaLogout(base::OnceClosure callback) {
  AccountConsistencyService* accountConsistencyService =
      ios::AccountConsistencyServiceFactory::GetForBrowserState(browser_state_);
  accountConsistencyService->RemoveChromeConnectedCookies(std::move(callback));
}
