/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_CRX_FILE_CRX_CREATOR_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_CRX_FILE_CRX_CREATOR_H_

#include "src/components/crx_file/crx_creator.h"  // IWYU pragma: export

#include <vector>

namespace crx_file {

CreatorResult CreateWithMultipleKeys(const base::FilePath& output_path,
                                     const base::FilePath& zip_path,
                                     std::vector<crypto::RSAPrivateKey*> keys);
}  // namespace crx_file

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_CRX_FILE_CRX_CREATOR_H_
