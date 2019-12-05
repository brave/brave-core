/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_SHOULD_SHOW_SIGNIN_PROMO_SHORT_CIRCUIT_TO_FALSE \
return false;

#include "../../../../../../../chrome/browser/ui/autofill/payments/save_card_bubble_controller_impl.cc"  // NOLINT
#undef BRAVE_SHOULD_SHOW_SIGNIN_PROMO_SHORT_CIRCUIT_TO_FALSE
