/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/signin/chrome_signin_client_factory.h"

// We must have a valid `IdentityManager` in OTR/guest profiles as it is passed
// by reference now, so we pass `kOwnInstance` to ensure that we do. This is
// needed for `IdentityManagerService` and all related classes (such as this
// one)
#define WithAshInternals(...)                      \
  WithAshInternals(__VA_ARGS__)                    \
      .WithRegular(ProfileSelection::kOwnInstance) \
      .WithGuest(ProfileSelection::kOwnInstance)

#include <chrome/browser/signin/chrome_signin_client_factory.cc>

#undef WithAshInternals
