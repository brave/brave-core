/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_IMMERSIVE_REVEALED_LOCK_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_IMMERSIVE_REVEALED_LOCK_H_

#include "base/component_export.h"
#include "base/memory/weak_ptr.h"

// Class which keeps the top-of-window views revealed for the duration of its
// lifetime. If acquiring the lock causes a reveal, the top-of-window views
// will animate according to the |animate_reveal| parameter passed in the
// constructor. See ImmersiveFullscreenController::GetRevealedLock() for more
// details.
class SimpleImmersiveRevealedLock {
 public:
  class Delegate {
   public:
    enum AnimateReveal { ANIMATE_REVEAL_YES, ANIMATE_REVEAL_NO };

    virtual void LockRevealedState(AnimateReveal animate_reveal) = 0;
    virtual void UnlockRevealedState() = 0;

   protected:
    virtual ~Delegate() {}
  };

  SimpleImmersiveRevealedLock(const base::WeakPtr<Delegate>& delegate,
                              Delegate::AnimateReveal animate_reveal);

  SimpleImmersiveRevealedLock(const SimpleImmersiveRevealedLock&) = delete;
  SimpleImmersiveRevealedLock& operator=(const SimpleImmersiveRevealedLock&) =
      delete;

  ~SimpleImmersiveRevealedLock();

 private:
  base::WeakPtr<Delegate> delegate_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_IMMERSIVE_REVEALED_LOCK_H_
