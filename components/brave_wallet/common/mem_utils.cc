/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/mem_utils.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_WIN)
#include <windows.h>
#endif

namespace brave_wallet {

void SecureZeroData(void* data, size_t size) {
  if (data == nullptr || size == 0) {
    return;
  }
#if BUILDFLAG(IS_WIN)
  SecureZeroMemory(data, size);
#else
  // 'volatile' is required. Otherwise optimizers can remove this function
  // if cleaning local variables, which are not used after that.
  volatile uint8_t* d = (volatile uint8_t*)data;
  for (size_t i = 0; i < size; i++) {
    d[i] = 0;
  }
#endif
}

}  // namespace brave_wallet
