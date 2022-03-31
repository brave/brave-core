/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_VARIATIONS_VARIATIONS_IDS_PROVIDER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_VARIATIONS_VARIATIONS_IDS_PROVIDER_H_

#define GetClientDataHeaders                                  \
  GetClientDataHeaders(bool is_signed_in) { return nullptr; } \
  variations::mojom::VariationsHeadersPtr GetClientDataHeaders_Chromium

#include "src/components/variations/variations_ids_provider.h"
#undef GetClientDataHeaders

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_VARIATIONS_VARIATIONS_IDS_PROVIDER_H_
