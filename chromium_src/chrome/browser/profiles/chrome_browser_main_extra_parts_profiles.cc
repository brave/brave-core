/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/browser_context_keyed_service_factories.h"
#include "chrome/browser/webdata_services/web_data_service_factory.h"

#define WebDataServiceFactory                              \
  brave::EnsureBrowserContextKeyedServiceFactoriesBuilt(); \
  WebDataServiceFactory

#include "src/chrome/browser/profiles/chrome_browser_main_extra_parts_profiles.cc"
#undef WebDataServiceFactory
