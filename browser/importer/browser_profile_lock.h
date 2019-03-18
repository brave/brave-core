/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IMPORTER_BROWSER_PROFILE_LOCK_H__
#define BRAVE_BROWSER_IMPORTER_BROWSER_PROFILE_LOCK_H__

class BrowserProfileLock {
 public:
  virtual ~BrowserProfileLock() {}

  // Locks and releases the profile.
  virtual void Lock() = 0;
  virtual void Unlock() = 0;

  // Returns true if we lock the profile successfully.
  virtual bool HasAcquired() = 0;
};

#endif  // BRAVE_BROWSER_IMPORTER_BROWSER_PROFILE_LOCK_H__
