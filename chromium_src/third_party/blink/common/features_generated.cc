/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"

#include <third_party/blink/common/features_generated.cc>

namespace blink::features {
OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kAIProofreadingAPI, base::FEATURE_DISABLED_BY_DEFAULT},
    {kAIPromptAPI, base::FEATURE_DISABLED_BY_DEFAULT},
    {kAIPromptAPIMultimodalInput, base::FEATURE_DISABLED_BY_DEFAULT},
    {kAIRewriterAPI, base::FEATURE_DISABLED_BY_DEFAULT},
    {kAISummarizationAPI, base::FEATURE_DISABLED_BY_DEFAULT},
    {kAIWriterAPI, base::FEATURE_DISABLED_BY_DEFAULT},
    {kLanguageDetectionAPI, base::FEATURE_DISABLED_BY_DEFAULT},
    {kTranslationAPI, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace blink::features
