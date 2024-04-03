/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/page_content_annotations/core/page_content_annotations_features.cc"

#include "base/feature_override.h"

namespace page_content_annotations::features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kPageContentAnnotations, base::FEATURE_DISABLED_BY_DEFAULT},
    {kPageVisibilityBatchAnnotations, base::FEATURE_DISABLED_BY_DEFAULT},
    {kPageVisibilityPageContentAnnotations, base::FEATURE_DISABLED_BY_DEFAULT},
    {kRemotePageMetadata, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace page_content_annotations::features
