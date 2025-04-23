/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from 'chrome://resources/js/load_time_data.js'
import { PluralStringProxyImpl } from 'chrome://resources/js/plural_string_proxy.js'
import { Locale } from '../models/locale_strings'

export function createLocale(): Locale {
  return {
    getString(key) {
      return loadTimeData.getString(key)
    },

    async getPluralString(key, count) {
      return PluralStringProxyImpl.getInstance().getPluralString(key, count)
    }
  }
}
