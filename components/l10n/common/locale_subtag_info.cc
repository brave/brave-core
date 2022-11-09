/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/common/locale_subtag_info.h"

namespace brave_l10n {

LocaleSubtagInfo::LocaleSubtagInfo() = default;

LocaleSubtagInfo::LocaleSubtagInfo(const LocaleSubtagInfo& other) = default;

LocaleSubtagInfo& LocaleSubtagInfo::operator=(const LocaleSubtagInfo& other) =
    default;

LocaleSubtagInfo::LocaleSubtagInfo(LocaleSubtagInfo&& other) noexcept = default;

LocaleSubtagInfo& LocaleSubtagInfo::operator=(
    LocaleSubtagInfo&& other) noexcept = default;

LocaleSubtagInfo::~LocaleSubtagInfo() = default;

bool operator==(const LocaleSubtagInfo& lhs, const LocaleSubtagInfo& rhs) {
  return lhs.language == rhs.language && lhs.script == rhs.script &&
         lhs.country == rhs.country && lhs.charset == rhs.charset &&
         lhs.variant == rhs.variant;
}

bool operator!=(const LocaleSubtagInfo& lhs, const LocaleSubtagInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_l10n
