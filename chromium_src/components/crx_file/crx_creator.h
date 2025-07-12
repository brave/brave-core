/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_CRX_FILE_CRX_CREATOR_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_CRX_FILE_CRX_CREATOR_H_

#include "base/containers/span.h"

#include "src/components/crx_file/crx_creator.h"  // IWYU pragma: export

namespace crx_file {

CreatorResult CreateWithMultipleKeys(
    const base::FilePath& output_path,
    const base::FilePath& zip_path,
    base::span<const crypto::keypair::PrivateKey> keys);
}  // namespace crx_file

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_CRX_FILE_CRX_CREATOR_H_
