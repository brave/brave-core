/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

interface Props {
  href: string
  children: React.ReactNode
}

export function NewTabLink (props: Props) {
  return (
    <a href={props.href} target='_blank' rel='noopener noreferrer'>
      {props.children}
    </a>
  )
}
