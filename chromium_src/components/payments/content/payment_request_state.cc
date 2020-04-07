// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "components/payments/content/payment_request.h"
#include "components/payments/core/journey_logger.h"

#define BRAVE_BAT_PAYMENT_UI_ON_PAYMENT_APP_CREATED                              \
    const char kBat[] = "bat";                                                   \
    if (base::StartsWith(available_apps_.back().get()->GetLabel(),               \
    	    base::ASCIIToUTF16(kBat), base::CompareCase::SENSITIVE)) {           \
        SetSelectedApp(available_apps_.back().get(),                             \
            SectionSelectionStatus::kAddedSelected);                             \
     }                                                                           \
  
#include "../../../../../components/payments/content/payment_request_state.cc"
#undef BRAVE_COMPLETE_PAYMENT_UI