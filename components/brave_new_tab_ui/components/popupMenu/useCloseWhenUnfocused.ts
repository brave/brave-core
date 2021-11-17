// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

export default function useCloseWhenUnfocused (ref: React.RefObject<HTMLUListElement>, onClose: () => unknown) {
  // Call onClose when
  // - click outside
  // - lose focus
  // - press esc
  React.useEffect(() => {
    if (!ref.current) {
      return
    }
    const handleFocusChange = (ev: FocusEvent) => {
      if (!ref.current) {
        return
      }
      const isFocusWithinMenu = (document.activeElement === ref.current) ||
        ref.current.contains(document.activeElement)
      console.log('is?', isFocusWithinMenu, document.activeElement)
      if (!isFocusWithinMenu) {
        onClose()
      }
    }
    const handleKeyDown = (ev: KeyboardEvent) => {
      if (ev.key === 'Escape') {
        onClose()
      }
    }
    const handleClick = (ev: MouseEvent) => {
      if (ref.current && ev.target instanceof HTMLElement && !ref.current.contains(ev.target)) {
        onClose()
      }
    }
    // window.addEventListener('blur', handleFocusChange, true)
    window.addEventListener('focus', handleFocusChange, true)
    window.addEventListener('keydown', handleKeyDown)
    window.addEventListener('click', handleClick)
    return () => {
      // window.removeEventListener('blur', handleFocusChange)
      window.removeEventListener('focus', handleFocusChange)
      window.removeEventListener('keydown', handleKeyDown)
      window.removeEventListener('click', handleClick)
    }
  }, [ref.current, onClose])
}
