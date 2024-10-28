/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from 'chrome://resources/js/load_time_data.js'

export function createLocale() {
  return {
    getString(key: string) {
      return loadTimeData.getString(key)
    },

    async getPluralString(key: string, count: number) {
      throw new Error('Not implemented')
    }
  }
}
