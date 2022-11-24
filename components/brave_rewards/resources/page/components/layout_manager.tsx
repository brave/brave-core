/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LayoutContext, LayoutKind } from '../lib/layout_context'

const layoutBreakpoint = 880

export function getPreferredLayout (): LayoutKind {
  return window.innerWidth > layoutBreakpoint ? 'wide' : 'narrow'
}

interface Props {
  layout?: LayoutKind
  children: React.ReactNode
}

export function LayoutManager (props: Props) {
  const [defaultLayout, setDefaultLayout] = React.useState(getPreferredLayout())
  const layoutKind = props.layout || defaultLayout

  React.useEffect(() => {
    const onResize = () => { setDefaultLayout(getPreferredLayout()) }
    window.addEventListener('resize', onResize)
    return () => { window.removeEventListener('resize', onResize) }
  }, [])

  return (
    <LayoutContext.Provider value={layoutKind}>
      <div className={`layout-${layoutKind}`}>
        {props.children}
      </div>
    </LayoutContext.Provider>
  )
}
