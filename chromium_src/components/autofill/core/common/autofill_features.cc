/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/autofill/core/common/autofill_features.cc"

#include "base/feature_override.h"

namespace autofill::features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {test::kAutofillServerCommunication, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace autofill::features
