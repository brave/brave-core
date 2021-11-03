/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../../third_party/blink/common/features.cc"

#include "base/feature_override.h"

namespace blink {
namespace features {

// Upgrade all mixed content
ENABLE_FEATURE_BY_DEFAULT(kMixedContentAutoupgrade);

ENABLE_FEATURE_BY_DEFAULT(kPrefetchPrivacyChanges);
ENABLE_FEATURE_BY_DEFAULT(kReducedReferrerGranularity);

DISABLE_FEATURE_BY_DEFAULT(kAdInterestGroupAPI);
DISABLE_FEATURE_BY_DEFAULT(kComputePressure);
DISABLE_FEATURE_BY_DEFAULT(kConversionMeasurement);
DISABLE_FEATURE_BY_DEFAULT(kFledge);
DISABLE_FEATURE_BY_DEFAULT(kHandwritingRecognitionWebPlatformApiFinch);
DISABLE_FEATURE_BY_DEFAULT(kInterestGroupStorage);
DISABLE_FEATURE_BY_DEFAULT(kInterestCohortAPIOriginTrial);
DISABLE_FEATURE_BY_DEFAULT(kInterestCohortFeaturePolicy);
DISABLE_FEATURE_BY_DEFAULT(kNavigatorPluginsFixed);
DISABLE_FEATURE_BY_DEFAULT(kParakeet);
DISABLE_FEATURE_BY_DEFAULT(kPrerender2);
DISABLE_FEATURE_BY_DEFAULT(kReportAllJavaScriptFrameworks);
DISABLE_FEATURE_BY_DEFAULT(kSpeculationRulesPrefetchProxy);
DISABLE_FEATURE_BY_DEFAULT(kTextFragmentAnchor);
DISABLE_FEATURE_BY_DEFAULT(kWebSQLInThirdPartyContextEnabled);

const base::Feature kFileSystemAccessAPI{"FileSystemAccessAPI",
                                         base::FEATURE_DISABLED_BY_DEFAULT};

}  // namespace features
}  // namespace blink
