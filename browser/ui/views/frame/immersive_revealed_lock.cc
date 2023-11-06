/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/immersive_revealed_lock.h"

SimpleImmersiveRevealedLock::SimpleImmersiveRevealedLock(
    const base::WeakPtr<Delegate>& delegate,
    Delegate::AnimateReveal animate_reveal)
    : delegate_(delegate) {
  delegate_->LockRevealedState(animate_reveal);
}

SimpleImmersiveRevealedLock::~SimpleImmersiveRevealedLock() {
  if (delegate_) {
    delegate_->UnlockRevealedState();
  }
}
