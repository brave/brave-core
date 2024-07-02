/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from 'chrome://resources/js/load_time_data.js'
import { PluralStringProxyImpl } from 'chrome://resources/js/plural_string_proxy.js'

export function createLocaleForWebUI() {
  return {
    getString(key: string): string {
      return loadTimeData.getString(key)
    },
    getPluralString(key: string, count: number): Promise<string> {
      return PluralStringProxyImpl.getInstance().getPluralString(key, count)
    }
  }
}
