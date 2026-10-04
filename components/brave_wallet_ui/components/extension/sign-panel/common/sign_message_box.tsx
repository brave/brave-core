// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import {
  FramedMessageBox,
  ScrollOverflowHint,
  SignMessageBoxFrame,
} from '../style'

interface Props {
  children: React.ReactNode
  height?: string
  width?: string
}

export function SignMessageBox(props: Props) {
  const { children, height, width } = props

  // Refs
  const messageBoxRef = React.useRef<HTMLDivElement>(null)

  // State
  const [showOverflowHint, setShowOverflowHint] = React.useState(false)

  // Methods
  const updateOverflowHint = React.useCallback(() => {
    const messageBox = messageBoxRef.current
    if (!messageBox) {
      return
    }
    const overflowing = messageBox.scrollHeight - messageBox.clientHeight > 4
    const atBottom =
      messageBox.scrollTop + messageBox.clientHeight
      >= messageBox.scrollHeight - 1
    setShowOverflowHint(overflowing && !atBottom)
  }, [])

  // Effects
  React.useLayoutEffect(() => {
    const messageBox = messageBoxRef.current
    if (!messageBox) {
      return
    }
    updateOverflowHint()
    messageBox.addEventListener('scroll', updateOverflowHint)
    let observer: ResizeObserver | undefined
    if (typeof ResizeObserver !== 'undefined') {
      observer = new ResizeObserver(updateOverflowHint)
      observer.observe(messageBox)
      if (messageBox.firstElementChild) {
        observer.observe(messageBox.firstElementChild)
      }
    }
    return () => {
      messageBox.removeEventListener('scroll', updateOverflowHint)
      observer?.disconnect()
    }
  }, [children, updateOverflowHint])

  return (
    <SignMessageBoxFrame
      height={height}
      width={width}
    >
      <FramedMessageBox ref={messageBoxRef}>{children}</FramedMessageBox>
      {showOverflowHint && <ScrollOverflowHint />}
    </SignMessageBoxFrame>
  )
}
