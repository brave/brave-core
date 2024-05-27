// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_REWRITER_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_AI_REWRITER_COMMON_FEATURES_H_

#include "base/component_export.h"
#include "base/feature_list.h"

namespace ai_rewriter::features {

COMPONENT_EXPORT(AI_REWRITER_COMMON) BASE_DECLARE_FEATURE(kAIRewriter);

COMPONENT_EXPORT(AI_REWRITER_COMMON) bool IsAIRewriterEnabled();

}  // namespace ai_rewriter::features

#endif  // BRAVE_COMPONENTS_AI_REWRITER_COMMON_FEATURES_H_
