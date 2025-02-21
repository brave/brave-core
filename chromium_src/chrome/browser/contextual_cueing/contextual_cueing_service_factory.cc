/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/contextual_cueing/contextual_cueing_service_factory.h"

#include "chrome/browser/profiles/profile_selections.h"

// ContextualCuelingService is used by Glic and uses
// PageContentExtractionService both of which we disable, so don't create this
// service.
#define kOriginalOnly kNone
#include "src/chrome/browser/contextual_cueing/contextual_cueing_service_factory.cc"
#undef kOriginalOnly
