/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import * as style from './media_card.style'

interface Props {
  title: React.ReactNode
  postTime: string
  icon: React.ReactNode
  children: React.ReactNode
}

export function MediaCard (props: Props) {
  return (
    <style.root>
      <style.header>
        <style.title>{props.title}</style.title>
        <style.date>
          {props.postTime} <style.icon>{props.icon}</style.icon>
        </style.date>
      </style.header>
      <style.text>{props.children}</style.text>
    </style.root>
  )
}
