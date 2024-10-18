/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/search_engine_choice/search_engine_choice_service_factory.h"

#include "chrome/browser/profiles/incognito_helpers.h"

#include "src/chrome/browser/search_engine_choice/search_engine_choice_service_factory.cc"

namespace search_engines {

content::BrowserContext*
SearchEngineChoiceServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // To make different service for normal and incognito profile.
  return GetBrowserContextOwnInstanceInIncognito(context);
}

}  // namespace search_engines
