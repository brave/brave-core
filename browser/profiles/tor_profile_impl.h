/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PROFILES_TOR_PROFILE_IMPL_H_
#define BRAVE_BROWSER_PROFILES_TOR_PROFILE_IMPL_H_

#include "chrome/browser/profiles/off_the_record_profile_impl.h"

class TorProfileImpl : public OffTheRecordProfileImpl {
 public:
  explicit TorProfileImpl(Profile* real_profile);
  ~TorProfileImpl() override;

  Profile* GetTorProfile() override;
  void DestroyTorProfile() override;
  bool HasTorProfile() override;
  bool IsTorProfile() const override;

  DISALLOW_COPY_AND_ASSIGN(TorProfileImpl);
};

#endif // BRAVE_BROWSER_PROFILES_TOR_PROFILE_IMPL_H_
