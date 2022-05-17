// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { copyToClipboard } from '../../utils/copy-to-clipboard'

const temporaryCopyTimeout = 5000 // 5s

export const useTemporaryCopyToClipboard = () => {
  // state
  const [isCopied, setIsCopied] = React.useState(false)

  // methods
  const temporaryCopyToClipboard = React.useCallback(async (value: string) => {
    await copyToClipboard(value)
    setIsCopied(true)
  }, [])

  // effects
  React.useEffect(() => {
    if (!isCopied) {
      // nothing to clear
      return () => {}
    }

    // clear the clipboard after a set time
    const timer = setTimeout(async () => {
      setIsCopied(false)
      await copyToClipboard('')
    }, temporaryCopyTimeout)

    // clean-up on unmount if timer was set
    return () => {
      clearTimeout(timer)
    }
  }, [isCopied])

  return temporaryCopyToClipboard
}
