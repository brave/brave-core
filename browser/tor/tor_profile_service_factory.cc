/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_profile_service_factory.h"

#include "base/check_is_test.h"
#include "base/no_destructor.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/tor/util.h"
#include "brave/components/tor/brave_tor_client_updater.h"
#include "brave/components/tor/brave_tor_pluggable_transport_updater.h"
#include "brave/components/tor/pref_names.h"
#include "brave/components/tor/tor_profile_service_impl.h"
#include "brave/components/tor/tor_utils.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"

namespace {


}  // namespace

// static
tor::TorProfileService* TorProfileServiceFactory::GetForContext(
    content::BrowserContext* context) {
  return GetForContext(context, true);
}

// static
tor::TorProfileService* TorProfileServiceFactory::GetForContext(
    content::BrowserContext* context,
    bool create) {
  return static_cast<tor::TorProfileService*>(
      GetInstance()->GetServiceForBrowserContext(context, create));
}

// static
TorProfileServiceFactory* TorProfileServiceFactory::GetInstance() {
  static base::NoDestructor<TorProfileServiceFactory> instance;
  return instance.get();
}

// static
void TorProfileServiceFactory::SetTorDisabled(bool disabled) {
  if (g_browser_process) {
    g_browser_process->local_state()->SetBoolean(tor::prefs::kTorDisabled,
                                                 disabled);
  }
}

// static
bool TorProfileServiceFactory::IsTorManaged(content::BrowserContext* context) {
  if (tor::IsIncognitoDisabledOrForced(context)) {
    return true;
  }
  if (g_browser_process) {
    return g_browser_process->local_state()
        ->FindPreference(tor::prefs::kTorDisabled)
        ->IsManaged();
  }
  return true;
}

// static
bool TorProfileServiceFactory::IsTorDisabled(content::BrowserContext* context) {
  if (Profile::FromBrowserContext(context)->IsGuestSession()) {
    return true;
  }
  if (tor::IsIncognitoDisabledOrForced(context)) {
    // Tor profile is derived from the incognito profile. If incognito is
    // disabled we can't create the tor profile. If incognito is forced then
    // browser forces incognito profile on creation (so created Tor profile
    // replaced by a new raw incognito profile).
    return true;
  }
  if (g_browser_process) {
    // local_state can be null in tests
    if (g_browser_process->local_state()) {
      return g_browser_process->local_state()->GetBoolean(
          tor::prefs::kTorDisabled);
    } else {
      CHECK_IS_TEST();
    }
  }
  return false;
}

// static
void TorProfileServiceFactory::SetTorBridgesConfig(
    const tor::BridgesConfig& config) {
  if (g_browser_process) {
    g_browser_process->local_state()->SetDict(tor::prefs::kBridgesConfig,
                                              config.ToDict());
  }
}

// static
tor::BridgesConfig TorProfileServiceFactory::GetTorBridgesConfig() {
  if (!g_browser_process) {
    return {};
  }
  return tor::BridgesConfig::FromDict(g_browser_process->local_state()->GetDict(
                                          tor::prefs::kBridgesConfig))
      .value_or(tor::BridgesConfig());
}

TorProfileServiceFactory::TorProfileServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "TorProfileService",
          BrowserContextDependencyManager::GetInstance()) {}

TorProfileServiceFactory::~TorProfileServiceFactory() = default;

KeyedService* TorProfileServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  tor::BraveTorClientUpdater* tor_client_updater = nullptr;
  tor::BraveTorPluggableTransportUpdater* tor_pluggable_transport_updater =
      nullptr;
  if (g_brave_browser_process) {
    tor_client_updater = g_brave_browser_process->tor_client_updater();
    tor_pluggable_transport_updater =
        g_brave_browser_process->tor_pluggable_transport_updater();
  }
  return new tor::TorProfileServiceImpl(
      Profile::FromBrowserContext(context)->GetOriginalProfile(), context,
      g_browser_process->local_state(), tor_client_updater,
      tor_pluggable_transport_updater);
}

content::BrowserContext* TorProfileServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // Only grant service for tor context
  if (!context->IsTor()) {
    return nullptr;
  }
  return context;
}
