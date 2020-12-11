/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_RPILL_RPILL_HELPER_H_
#define BAT_ADS_INTERNAL_RPILL_RPILL_HELPER_H_

#include "base/memory/singleton.h"

namespace ads {

class RPillHelper {
 public:
  RPillHelper(const RPillHelper&) = delete;
  RPillHelper& operator=(const RPillHelper&) = delete;

  static RPillHelper* GetInstance();

  void set_for_testing(
      RPillHelper* rpill_helper);

  virtual bool IsUncertainFuture() const;

 protected:
  friend struct base::DefaultSingletonTraits<RPillHelper>;

  RPillHelper();
  virtual ~RPillHelper();

  static RPillHelper* GetInstanceImpl();
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_RPILL_RPILL_HELPER_H_
