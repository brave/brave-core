/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_SYS_INFO_HELPER_H_
#define BAT_ADS_INTERNAL_SYS_INFO_HELPER_H_

#include "base/memory/singleton.h"
#include "base/one_shot_event.h"
#include "base/system/sys_info.h"

namespace ads {

class SysInfoHelper {
 public:
  SysInfoHelper(const SysInfoHelper&) = delete;
  SysInfoHelper& operator=(const SysInfoHelper&) = delete;

  static SysInfoHelper* GetInstance();

  void set_for_testing(
      SysInfoHelper* sys_info_helper);

  virtual void Initialize();

  const base::OneShotEvent& ready() const;

  bool is_ready() const;

  virtual base::SysInfo::HardwareInfo GetHardware() const;

 protected:
  friend struct base::DefaultSingletonTraits<SysInfoHelper>;

  SysInfoHelper();

  virtual ~SysInfoHelper();

  static SysInfoHelper* GetInstanceImpl();

  base::SysInfo::HardwareInfo hardware_;

  base::OneShotEvent ready_;

 private:
  void OnGetHardwareInfo(
      base::SysInfo::HardwareInfo hardware);
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_SYS_INFO_HELPER_H_
