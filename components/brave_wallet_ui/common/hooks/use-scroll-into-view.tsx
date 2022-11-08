// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

export function useScrollIntoView () {
  // Prevents scrolling into view again if re-render occurs
  const alreadyScrolled = React.useRef(false)
  const scrollIntoView = React.useCallback((ref: HTMLElement | null) => {
    if (!alreadyScrolled.current) {
      ref?.scrollIntoView({
        behavior: 'smooth',
        block: 'center',
        inline: 'center'
      })
      alreadyScrolled.current = true
    }
  }, [alreadyScrolled])

  return scrollIntoView
}

export default useScrollIntoView
