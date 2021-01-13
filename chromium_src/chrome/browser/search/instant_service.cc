/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/search/instant_service.h"
#include "components/search/search_provider_observer.h"

class AlwaysGoogleSearchProviderObserver : public SearchProviderObserver {
 public:
  using SearchProviderObserver::SearchProviderObserver;
  bool is_google() override { return true; }
};

#define SearchProviderObserver AlwaysGoogleSearchProviderObserver
#include "../../../../../chrome/browser/search/instant_service.cc"
#undef SearchProviderObserver
