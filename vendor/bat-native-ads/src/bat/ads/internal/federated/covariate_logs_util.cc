/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/federated/covariate_logs_util.h"

#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"

namespace {

constexpr char kTrue[] = "true";
constexpr char kFalse[] = "false";

}  // namespace

namespace ads {

std::string ToString(const int value) {
  return base::NumberToString(value);
}

std::string ToString(const int64_t value) {
  return base::NumberToString(value);
}

std::string ToString(const double value) {
  return base::NumberToString(value);
}

std::string ToString(const bool value) {
  return value ? kTrue : kFalse;
}

std::string ToString(const base::Time time) {
  DCHECK(!time.is_null());
  return base::NumberToString(time.ToDoubleT());
}

}  // namespace ads
