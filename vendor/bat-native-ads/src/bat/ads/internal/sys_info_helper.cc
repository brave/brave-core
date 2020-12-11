/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/sys_info_helper.h"

#include "base/bind.h"

namespace ads {

SysInfoHelper* g_sys_info_helper_for_testing = nullptr;

SysInfoHelper::SysInfoHelper() = default;

SysInfoHelper::~SysInfoHelper() = default;

void SysInfoHelper::set_for_testing(
    SysInfoHelper* sys_info_helper) {
  g_sys_info_helper_for_testing = sys_info_helper;
}

void SysInfoHelper::Initialize() {
  base::SysInfo::GetHardwareInfo(base::BindOnce(
      &SysInfoHelper::OnGetHardwareInfo, base::Unretained(this)));
}

const base::OneShotEvent& SysInfoHelper::ready() const {
  return ready_;
}

bool SysInfoHelper::is_ready() const {
  return ready_.is_signaled();
}

base::SysInfo::HardwareInfo SysInfoHelper::GetHardware() const {
  return hardware_;
}

SysInfoHelper* SysInfoHelper::GetInstance() {
  if (g_sys_info_helper_for_testing) {
    return g_sys_info_helper_for_testing;
  }

  return GetInstanceImpl();
}

SysInfoHelper* SysInfoHelper::GetInstanceImpl() {
  return base::Singleton<SysInfoHelper>::get();
}

void SysInfoHelper::OnGetHardwareInfo(
    base::SysInfo::HardwareInfo hardware) {
  hardware_ = hardware;

  if (ready_.is_signaled()) {
    return;
  }

  ready_.Signal();
}

}  // namespace ads
