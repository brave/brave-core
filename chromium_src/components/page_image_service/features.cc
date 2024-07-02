/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/page_image_service/features.cc"

#include "base/feature_override.h"

namespace page_image_service {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kImageService, base::FEATURE_DISABLED_BY_DEFAULT},
    {kImageServiceSuggestPoweredImages, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace page_image_service
