/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/search_engines/template_url_service_factory.h"

#define WithAshInternals(...)                      \
  WithAshInternals(__VA_ARGS__)                    \
      .WithRegular(ProfileSelection::kOwnInstance) \
      .WithGuest(ProfileSelection::kOwnInstance)

#include "src/chrome/browser/search_engines/template_url_service_factory.cc"

#undef WithAshInternals
