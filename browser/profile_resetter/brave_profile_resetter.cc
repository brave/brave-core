/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profile_resetter/brave_profile_resetter.h"

#include "brave/browser/search_engines/search_engine_provider_util.h"

BraveProfileResetter::~BraveProfileResetter() = default;

void BraveProfileResetter::ResetDefaultSearchEngine() {
  ProfileResetter::ResetDefaultSearchEngine();

  if (template_url_service_->loaded()) {
    brave::ResetDefaultPrivateSearchProvider(profile_);
  }
}
