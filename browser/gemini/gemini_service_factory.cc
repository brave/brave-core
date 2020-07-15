/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/gemini/gemini_service_factory.h"

#include "brave/browser/profiles/profile_util.h"
#include "brave/components/gemini/browser/gemini_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

// static
GeminiServiceFactory* GeminiServiceFactory::GetInstance() {
  return base::Singleton<GeminiServiceFactory>::get();
}

// static
GeminiService* GeminiServiceFactory::GetForProfile(Profile* profile) {
  if (brave::IsTorProfile(profile) ||
      profile->IsIncognitoProfile() ||
      profile->IsGuestSession()) {
    return nullptr;
  }

  return static_cast<GeminiService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

GeminiServiceFactory::GeminiServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "GeminiService",
          BrowserContextDependencyManager::GetInstance()) {
}

GeminiServiceFactory::~GeminiServiceFactory() {
}

KeyedService* GeminiServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new GeminiService(Profile::FromBrowserContext(context));
}

content::BrowserContext* GeminiServiceFactory::GetBrowserContextToUse(
      content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}
