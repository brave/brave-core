/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_LOGGING_H_
#define BAT_CONFIRMATIONS_LOGGING_H_

#include "bat/confirmations/confirmations_client.h"

#define BLOG(severity) \
  confirmations_client_->Log(__FILE__, __LINE__, severity)->stream()

#define BVLOG(severity) \
  confirmations_client_->VerboseLog(__FILE__, __LINE__, severity)->stream()

#endif  // BAT_CONFIRMATIONS_LOGGING_H_
