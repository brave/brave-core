// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "components/payments/content/payment_request.h"
#include "components/payments/core/journey_logger.h"

using payments::JourneyLogger;

#define BRAVE_BAT_PAYMENT_UI_PAY                                    \
  if (spec()->payment_method_identifiers_set().count("bat")) { \
    return; \
  } \

#include "../../../../../components/payments/content/payment_request.cc"
#undef BRAVE_COMPLETE_PAYMENT_UI