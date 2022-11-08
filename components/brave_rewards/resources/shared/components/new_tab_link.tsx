/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

interface Props {
  href: string
  children: React.ReactNode
}

export function NewTabLink (props: Props) {
  const onClick = (event: React.UIEvent) => {
    // If the `chrome.tabs` API is available, then use it to open the new tab.
    // This will allow links to be opened from contexts where normal links do
    // not work (e.g. a bubble).
    if (chrome.tabs) {
      event.preventDefault()
      chrome.tabs.create({ url: props.href })
    }
  }

  return (
    <a
      href={props.href}
      onClick={onClick}
      target='_blank'
      rel='noopener noreferrer'
    >
      {props.children}
    </a>
  )
}
