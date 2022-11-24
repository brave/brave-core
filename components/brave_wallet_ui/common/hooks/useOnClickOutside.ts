// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

type Event = MouseEvent | TouchEvent

export const useOnClickOutside = <T extends HTMLElement = HTMLElement> (
  ref: React.RefObject<T>,
  handler: (event: Event) => void,
  startListening: boolean,
  // Include a buttonId if you have a button outside of the click away area
  // that causes reopening when clicking to close.
  buttonId?: string
) => {
  React.useEffect(() => {
    const listener = (event: Event) => {
      if ((!ref.current || ref.current.contains((event?.target as Node) || null) ||
        (buttonId === (event.target as HTMLButtonElement).id))
      ) {
        return
      }
      handler(event)
    }
    if (startListening) {
      document.addEventListener('mousedown', listener)
      document.addEventListener('touchstart', listener)
      return () => {
        document.removeEventListener('mousedown', listener)
        document.removeEventListener('touchstart', listener)
      }
    }
    return () => {
      document.removeEventListener('mousedown', listener)
      document.removeEventListener('touchstart', listener)
    }
  }, [ref, startListening, handler, buttonId])
}
