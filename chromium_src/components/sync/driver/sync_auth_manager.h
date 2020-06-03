/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DRIVER_SYNC_AUTH_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DRIVER_SYNC_AUTH_MANAGER_H_

#define BRAVE_SYNC_AUTH_MANAGER_H_                                          \
  void DeriveSigningKeys(const std::string& seed);                          \
  void ResetKeys();                                                         \
                                                                            \
 private:                                                                   \
  std::string GenerateAccessToken(const std::string& timestamp);            \
  std::vector<uint8_t> public_key_;                                         \
  std::vector<uint8_t> private_key_;

#include "../../../../../components/sync/driver/sync_auth_manager.h"

#undef BRAVE_SYNC_AUTH_MANAGER_H_
#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DRIVER_SYNC_AUTH_MANAGER_H_
