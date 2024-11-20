/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/user_education/user_education_service_factory.h"

#include "chrome/browser/profiles/profile_selections.h"

// Do not create education service for regular and guest profiles.
#define WithRegular(SELECTION) WithRegular(ProfileSelection::kNone)
#define WithGuest(SELECTION) WithGuest(ProfileSelection::kNone)

#include "src/chrome/browser/user_education/user_education_service_factory.cc"
#undef WithGuest
#undef WithRegular
