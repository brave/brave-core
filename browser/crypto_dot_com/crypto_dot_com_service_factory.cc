/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/crypto_dot_com/crypto_dot_com_service_factory.h"

#include "brave/browser/profiles/profile_util.h"
#include "brave/components/crypto_dot_com/browser/crypto_dot_com_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

// static
CryptoDotComServiceFactory* CryptoDotComServiceFactory::GetInstance() {
  return base::Singleton<CryptoDotComServiceFactory>::get();
}

// static
CryptoDotComService* CryptoDotComServiceFactory::GetForProfile(Profile* profile) {
  if (brave::IsTorProfile(profile) ||
      profile->IsIncognitoProfile() ||
      profile->IsGuestSession()) {
    return nullptr;
  }

  return static_cast<CryptoDotComService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

CryptoDotComServiceFactory::CryptoDotComServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "CryptoDotComService",
          BrowserContextDependencyManager::GetInstance()) {
}

CryptoDotComServiceFactory::~CryptoDotComServiceFactory() {
}

KeyedService* CryptoDotComServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new CryptoDotComService(Profile::FromBrowserContext(context));
}

content::BrowserContext* CryptoDotComServiceFactory::GetBrowserContextToUse(
      content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}
