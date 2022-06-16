/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PROFILE_RESETTER_BRAVE_PROFILE_RESETTER_H_
#define BRAVE_BROWSER_PROFILE_RESETTER_BRAVE_PROFILE_RESETTER_H_

#include "chrome/browser/profile_resetter/profile_resetter.h"

// Reset brave specific prefs.
class BraveProfileResetter : public ProfileResetter {
 public:
  using ProfileResetter::ProfileResetter;
  BraveProfileResetter(const BraveProfileResetter&) = delete;
  BraveProfileResetter& operator=(const BraveProfileResetter&) = delete;
  ~BraveProfileResetter() override;

  // ProfileResetter overrides:
  void ResetDefaultSearchEngine() override;
};

#endif  // BRAVE_BROWSER_PROFILE_RESETTER_BRAVE_PROFILE_RESETTER_H_
