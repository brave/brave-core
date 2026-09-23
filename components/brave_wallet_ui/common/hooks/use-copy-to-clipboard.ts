// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { copyToClipboard } from '../../utils/copy-to-clipboard'
import {
  useCopyToClipboardConfidentiallyMutation, //
} from '../slices/api.slice'

const temporaryCopyTimeout = 5000 // 5s
const copiedMessageTimeout = 1500 // 1.5s

export const useTemporaryCopyToClipboard = (
  timeoutMs: number = temporaryCopyTimeout,
  isConfidential = false,
) => {
  // mutations
  const [copyToClipboardConfidentially] =
    useCopyToClipboardConfidentiallyMutation()

  // state
  const [isCopied, setIsCopied] = React.useState(false)

  // methods
  const temporaryCopyToClipboard = React.useCallback(
    async (value: string) => {
      if (isConfidential) {
        await copyToClipboardConfidentially({ text: value })
      } else {
        await copyToClipboard(value)
      }
      setIsCopied(true)
    },
    [isConfidential, copyToClipboardConfidentially],
  )

  // effects
  React.useEffect(() => {
    if (!isCopied) {
      // nothing to clear
      return () => {}
    }

    // clear the clipboard after a set time
    const timer = window.setTimeout(async () => {
      if (isConfidential) {
        await copyToClipboardConfidentially({ text: '' })
      } else {
        await copyToClipboard('')
      }
      setIsCopied(false)
    }, timeoutMs)

    // clean-up on unmount if timer was set
    return () => {
      timer && clearTimeout(timer)
    }
  }, [isCopied, timeoutMs, isConfidential, copyToClipboardConfidentially])

  return {
    temporaryCopyToClipboard,
    isCopied,
  }
}

export const useCopyToClipboard = (
  timeoutMs = copiedMessageTimeout,
  isConfidential = false,
) => {
  // mutations
  const [copyToClipboardConfidentially] =
    useCopyToClipboardConfidentiallyMutation()

  // state
  const [isCopied, setIsCopied] = React.useState(false)

  // methods
  const _copyToClipboard = React.useCallback(
    async (value: string) => {
      if (isConfidential) {
        await copyToClipboardConfidentially({ text: value })
      } else {
        await copyToClipboard(value)
      }
      setIsCopied(true)
    },
    [isConfidential, copyToClipboardConfidentially],
  )

  const resetCopyState = React.useCallback(() => {
    setIsCopied(false)
  }, [])

  // effects
  React.useEffect(() => {
    if (!isCopied) {
      // nothing to clear
      return () => {}
    }

    // clear the message after a set time
    const timer = window.setTimeout(async () => {
      resetCopyState()
    }, timeoutMs)

    // clean-up on unmount if timer was set
    return () => {
      timer && clearTimeout(timer)
    }
  }, [isCopied, resetCopyState, timeoutMs])

  return {
    copyToClipboard: _copyToClipboard,
    resetCopyState,
    isCopied,
  }
}
