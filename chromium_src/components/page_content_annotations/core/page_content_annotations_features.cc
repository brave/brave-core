/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/page_content_annotations/core/page_content_annotations_features.h"

#define ShouldEnablePageContentAnnotations \
  ShouldEnablePageContentAnnotations_ChromiumImpl

#include <components/page_content_annotations/core/page_content_annotations_features.cc>
#undef ShouldEnablePageContentAnnotations

namespace page_content_annotations::features {

bool ShouldEnablePageContentAnnotations() {
  return false;
}

}  // namespace page_content_annotations::features
