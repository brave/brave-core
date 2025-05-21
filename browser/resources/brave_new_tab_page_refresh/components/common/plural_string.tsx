/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { PluralStringProxyImpl } from 'chrome://resources/js/plural_string_proxy.js'
import { PluralStringKey } from '../../lib/strings'

interface Props {
  stringKey: PluralStringKey
  count: number
}

// A React component that displays a plural string.
export function PluralString(props: Props) {
  return usePluralString(props.stringKey, props.count)
}

function usePluralString(key: PluralStringKey, count: number) {
  const [value, setValue] = React.useState('')

  React.useEffect(() => {
    if (typeof count !== 'number') {
      setValue('')
      return
    }
    let canUpdate = true
    PluralStringProxyImpl
      .getInstance()
      .getPluralString(key, count)
      .then((newValue) => {
        if (canUpdate) {
          setValue(newValue)
        }
      })
    return () => { canUpdate = false }
  }, [key, count])

  return value
}
