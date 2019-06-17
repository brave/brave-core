/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/tor_profile_impl.h"

TorProfileImpl::TorProfileImpl(Profile* real_profile)
  : OffTheRecordProfileImpl(real_profile) {
}

TorProfileImpl::~TorProfileImpl() {
}

Profile* TorProfileImpl::GetTorProfile() {
  return this;
}

void TorProfileImpl::DestroyTorProfile() {
  // Suicide is bad!
  NOTREACHED();
}

bool TorProfileImpl::HasTorProfile() {
  return true;
}

bool TorProfileImpl::IsTorProfile() const {
  return true;
}
