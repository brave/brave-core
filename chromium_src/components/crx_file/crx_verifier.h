/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_CRX_FILE_CRX_VERIFIER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_CRX_FILE_CRX_VERIFIER_H_

#include "base/containers/span.h"

#include "src/components/crx_file/crx_verifier.h"  // IWYU pragma: export

namespace crx_file {

void SetBravePublisherKeyHashForTesting(base::span<const uint8_t> test_key);

}  // namespace crx_file

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_CRX_FILE_CRX_VERIFIER_H_
