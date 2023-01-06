/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { PluralStringProxyImpl } from 'chrome://resources/js/plural_string_proxy.js'

export function createLocaleContextForWebUI () {
  return {
    getString (key: string): string {
      return (window as any).loadTimeData.getString(key)
    },
    getPluralString (
      key: string,
      count: number,
      callback: (result: string) => void
    ) {
      let maybeCallback: ((value: string) => void) | null = callback
      PluralStringProxyImpl
        .getInstance()
        .getPluralString(key, count)
        .then((value: string) => { maybeCallback && maybeCallback(value) })
      return () => { maybeCallback = null }
    }
  }
}
