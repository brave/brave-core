/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

type ViewType = 'narrow' | 'wide'

let breakpointQuery = window.matchMedia('(width < 530px)')

function getViewType() {
  return breakpointQuery.matches ? 'narrow' : 'wide'
}

// Returns the current view type, based upon the panel/page breakpoint. The
// view type can be used to switch between layouts when the window dimensions
// have changed.
export function useBreakpoint() {
  const [view, setView] = React.useState<ViewType>(getViewType())

  React.useEffect(() => {
    const listener = () => setView(getViewType())
    breakpointQuery.addEventListener('change', listener)
    return () => breakpointQuery.removeEventListener('change', listener)
  }, [])

  return view
}
