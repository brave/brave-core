/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPONSORED_CONTENT_SPONSORED_CONTENT_COMPONENT_CALLBACK_H_
#define BRAVE_COMPONENTS_SPONSORED_CONTENT_SPONSORED_CONTENT_COMPONENT_CALLBACK_H_

#include "base/functional/callback.h"

namespace base {
class FilePath;
}  // namespace base

namespace sponsored_content {

using ComponentCallback =
    base::RepeatingCallback<void(const base::FilePath& install_path)>;

}  // namespace sponsored_content

#endif  // BRAVE_COMPONENTS_SPONSORED_CONTENT_SPONSORED_CONTENT_COMPONENT_CALLBACK_H_
