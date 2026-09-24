/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { PluralStringProxyImpl } from 'chrome://resources/js/plural_string_proxy.js'

export function usePluralString(key: string, count: number) {
  const [pluralString, setPluralString] = React.useState('')
  React.useEffect(() => {
    PluralStringProxyImpl.getInstance()
      .getPluralString(key, count)
      .then(setPluralString)
  }, [key, count])
  return pluralString
}
