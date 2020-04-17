/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_PLATFORM_HELPER_H_
#define BAT_CONFIRMATIONS_INTERNAL_PLATFORM_HELPER_H_

#include <string>

#include "base/memory/singleton.h"

namespace confirmations {

class PlatformHelper {
 public:
  PlatformHelper(const PlatformHelper&) = delete;
  PlatformHelper& operator=(const PlatformHelper&) = delete;

  static PlatformHelper* GetInstance();

  void set_for_testing(
      PlatformHelper* platform_helper);

  virtual std::string GetPlatformName() const;

 protected:
  friend struct base::DefaultSingletonTraits<PlatformHelper>;

  PlatformHelper();
  virtual ~PlatformHelper();

  static PlatformHelper* GetInstanceImpl();
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_PLATFORM_HELPER_H_
