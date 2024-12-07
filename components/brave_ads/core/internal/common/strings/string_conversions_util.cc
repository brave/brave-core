/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/strings/string_conversions_util.h"

namespace brave_ads {

namespace {

constexpr char kTrue[] = "true";
constexpr char kFalse[] = "false";

}  // namespace

std::string BoolToString(bool value) {
  return value ? kTrue : kFalse;
}

}  // namespace brave_ads
