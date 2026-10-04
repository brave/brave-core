// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

const INPUT_TAGS = new Set(['INPUT', 'TEXTAREA'])

function isTextInputElement(el: Element | null): boolean {
  if (!el) return false
  if (INPUT_TAGS.has(el.tagName)) {
    // Exclude input types that don't trigger a software keyboard.
    const type = (el as HTMLInputElement).type
    return type !== 'checkbox' && type !== 'radio' && type !== 'button'
  }
  return el instanceof HTMLElement && el.isContentEditable
}

/**
 * Detects whether a software keyboard is likely visible by tracking
 * focus on text input elements. On mobile, focusing a text input
 * triggers the software keyboard.
 *
 * This is only meaningful when combined with a mobile check —
 * on desktop, inputs receive focus without a software keyboard.
 */
export const useIsKeyboardVisible = (): boolean => {
  const [isKeyboardVisible, setIsKeyboardVisible] = React.useState(() =>
    isTextInputElement(document.activeElement),
  )

  React.useEffect(() => {
    // Catch focus that occurred between the initial state snapshot and
    // the effect running (e.g. autofocus on a sibling component).
    if (isTextInputElement(document.activeElement)) {
      setIsKeyboardVisible(true)
    }

    const onFocusIn = (e: FocusEvent) => {
      if (isTextInputElement(e.target as Element | null)) {
        setIsKeyboardVisible(true)
      }
    }

    const onFocusOut = () => {
      // Small delay to handle focus moving between inputs without
      // briefly flashing the sticky button.
      setTimeout(() => {
        if (!isTextInputElement(document.activeElement)) {
          setIsKeyboardVisible(false)
        }
      }, 100)
    }

    document.addEventListener('focusin', onFocusIn)
    document.addEventListener('focusout', onFocusOut)

    return () => {
      document.removeEventListener('focusin', onFocusIn)
      document.removeEventListener('focusout', onFocusOut)
    }
  }, [])

  return isKeyboardVisible
}

export default useIsKeyboardVisible
