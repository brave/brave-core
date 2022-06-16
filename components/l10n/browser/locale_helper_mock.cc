/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/browser/locale_helper_mock.h"

namespace brave_l10n {

LocaleHelperMock::LocaleHelperMock() {
  brave_l10n::LocaleHelper::SetForTesting(this);
}

LocaleHelperMock::~LocaleHelperMock() {
  brave_l10n::LocaleHelper::SetForTesting(nullptr);
}

}  // namespace brave_l10n
