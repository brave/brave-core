/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_UPDATER_CONFIGURATOR_H_
#define BRAVE_CHROMIUM_SRC_CHROME_UPDATER_CONFIGURATOR_H_

#define EnabledCupSigning           \
  EnabledCupSigning_Unused() const; \
  bool EnabledCupSigning

#include <chrome/updater/configurator.h>  // IWYU pragma: export

#undef EnabledCupSigning

#endif  // BRAVE_CHROMIUM_SRC_CHROME_UPDATER_CONFIGURATOR_H_
