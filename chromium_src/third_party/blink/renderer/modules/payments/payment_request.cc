/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/payments/payment_request.h"

#define BRAVE_TYPE_CONVERTER_PAYMENT_ITEM output->sku = input.sku();
#include "../../../../../../../third_party/blink/renderer/modules/payments/payment_request.cc"
#undef BRAVE_TYPE_CONVERTER_PAYMENT_ITEM
