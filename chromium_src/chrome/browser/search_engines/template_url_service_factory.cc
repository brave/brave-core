/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/webdata_services/web_data_service_factory.h"

#include "src/chrome/browser/search_engines/template_url_service_factory.cc"

content::BrowserContext* TemplateURLServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // To make different service for normal and incognito profile.
  return chrome::GetBrowserContextOwnInstanceInIncognito(context);
}
