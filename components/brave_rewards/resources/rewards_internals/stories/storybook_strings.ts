/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { StringKey } from '../lib/locale_strings'

export const localeStrings: { [K in StringKey]: string } = {
  fullLogDisclaimerText:
    'WARNING: This log file may contain sensitive data. Be careful who you share it with.',
  pageDisclaimerText:
    'WARNING: Data on these pages may be sensitive. Be careful who you share them with.',
  pageTitle: 'Rewards internals',
}
