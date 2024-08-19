/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

interface TabOpener {
  openTab: (url: string) => void
}

const defaultTabOpener = {
  openTab (url: string) {
    window.open(url, '_blank', 'noopener,noreferrer')
  }
}

export const TabOpenerContext = React.createContext<TabOpener>(defaultTabOpener)

interface Props {
  href: string
  className?: string
  children: React.ReactNode
}

export function NewTabLink (props: Props) {
  const tabOpener = React.useContext(TabOpenerContext)

  const onClick = (event: React.UIEvent) => {
    // If a tab opener has been specified for this component tree, then use it
    // to open the new tab. This will allow links to be opened from contexts
    // where normal links do not work (e.g. a bubble).
    if (tabOpener !== defaultTabOpener) {
      event.preventDefault()
      tabOpener.openTab(props.href)
    }
  }

  return (
    <a
      href={props.href}
      onClick={onClick}
      className={props.className}
      target='_blank'
      rel='noopener noreferrer'
    >
      {props.children}
    </a>
  )
}
