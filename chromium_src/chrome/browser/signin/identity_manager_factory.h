/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SIGNIN_IDENTITY_MANAGER_FACTORY_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SIGNIN_IDENTITY_MANAGER_FACTORY_H_

class BraveIdentityManagerFactory;

#define BRAVE_IDENTITY_MANAGER_FACTORY_H_   \
 private:                                   \
  friend class BraveIdentityManagerFactory; \
  friend struct base::DefaultSingletonTraits<BraveIdentityManagerFactory>;

#include "../../../../../chrome/browser/signin/identity_manager_factory.h"

#undef BRAVE_IDENTITY_MANAGER_FACTORY_H_

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SIGNIN_IDENTITY_MANAGER_FACTORY_H_
