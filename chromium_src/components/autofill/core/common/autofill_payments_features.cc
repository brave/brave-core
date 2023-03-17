/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/autofill/core/common/autofill_payments_features.cc"

#include "base/feature_override.h"

namespace autofill {
namespace features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kAutofillEnableOfferNotificationForPromoCodes,
     base::FEATURE_DISABLED_BY_DEFAULT},
    {kAutofillEnableRemadeDownstreamMetrics, base::FEATURE_DISABLED_BY_DEFAULT},
    {kAutofillUpstreamAllowAdditionalEmailDomains,
     base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace features
}  // namespace autofill
