/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PROFILES_BRAVE_PROFILE_IMPL_H_
#define BRAVE_BROWSER_PROFILES_BRAVE_PROFILE_IMPL_H_

#include "chrome/browser/profiles/profile_impl.h"

#include "base/memory/weak_ptr.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class PrefStore;

class BraveProfileImpl : public ProfileImpl,
                         public content::NotificationObserver {
 public:
  BraveProfileImpl(const base::FilePath& path,
                   Delegate* delegate,
                   CreateMode create_mode,
                   scoped_refptr<base::SequencedTaskRunner> io_task_runner);
  ~BraveProfileImpl() override;

  // content::NotificationObserver override.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

 private:

  // Listens for parent profile destruction.
  content::NotificationRegistrar notification_registrar_;

  base::WeakPtrFactory<BraveProfileImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BraveProfileImpl);
};

#endif  // BRAVE_BROWSER_PROFILES_BRAVE_PROFILE_IMPL_H_
