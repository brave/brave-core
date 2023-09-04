/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/locale/locale_util.h"

#include "brave/components/l10n/common/locale_util.h"

namespace brave_ads {

const std::string& GetLocale() {
  return brave_l10n::GetDefaultLocaleString();
}

}  // namespace brave_ads
