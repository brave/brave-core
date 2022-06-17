// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { copyToClipboard } from '../../utils/copy-to-clipboard'

export const useCopy = () => {
  const [copied, setCopied] = React.useState<boolean>(false)

  const copyText = async (text: string) => {
    await copyToClipboard(text)
    setCopied(true)
  }

  React.useEffect(() => {
    let timer: number

    if (copied) {
      timer = window.setTimeout(() => {
        setCopied(false)
      }, 1500)
    }

    return () => {
      timer && clearTimeout(timer)
    }
  }, [copied])

  return {
    copied,
    copyText
  }
}
