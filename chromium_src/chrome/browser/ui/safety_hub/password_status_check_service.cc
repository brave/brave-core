/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/safety_hub/password_status_check_service.h"

// We hide the password card in brave://settings/safetyCheck, so we don't want
// to return recommendations that involve that card
#define BRAVE_PASSWORD_STATUS_CHECK_SERVICE_GET_PASSWORD_CARD_DATA   \
  base::DictValue dict;                                              \
  dict.Set(safety_hub::kCardStateKey,                                \
           static_cast<int>(safety_hub::SafetyHubCardState::kSafe)); \
  return dict;

#include <chrome/browser/ui/safety_hub/password_status_check_service.cc>

#undef BRAVE_PASSWORD_STATUS_CHECK_SERVICE_GET_PASSWORD_CARD_DATA
