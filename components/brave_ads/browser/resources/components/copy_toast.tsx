/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { createPortal } from 'react-dom'
import Alert from '@brave/leo/react/alert'

import { style } from './copy_toast.style'

const TOAST_DURATION_MS = 2000

const ShowToastContext = React.createContext((_message: string) => {})

// Reused by any tab that renders a copyable ID/URL (see
// `renderCopyableText` in `lib/copyable_text.tsx`) so clicking one always
// gives the same clipboard + toast feedback, regardless of which tab it's
// clicked from.
export function useCopyToClipboard() {
  const showToast = React.useContext(ShowToastContext)
  return React.useCallback((text: string) => {
    navigator.clipboard.writeText(text)
    showToast('Copied to clipboard.')
  }, [showToast])
}

// For toast feedback unrelated to copying (e.g. "Log cleared."), sharing the
// same toast so it looks and behaves identically everywhere.
export function useToast() {
  return React.useContext(ShowToastContext)
}

export function CopyToastProvider({ children }: { children: React.ReactNode }) {
  const [message, setMessage] = React.useState<string | null>(null)

  React.useEffect(() => {
    if (message === null) {
      return
    }
    const timeout = setTimeout(() => setMessage(null), TOAST_DURATION_MS)
    return () => clearTimeout(timeout)
  }, [message])

  const showToast = React.useCallback((text: string) => {
    setMessage(text)
  }, [])

  return (
    <ShowToastContext.Provider value={showToast}>
      {children}
      {message !== null &&
        createPortal(
          <Alert
            className='toast'
            data-css-scope={style.scope}
            type='success'
            role='status'
            aria-live='polite'
          >
            {message}
          </Alert>,
          document.body,
        )}
    </ShowToastContext.Provider>
  )
}
