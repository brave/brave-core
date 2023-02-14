/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { InfoBoxIcon } from './icons/info_box_icon'

import * as style from './info_box.style'

type InfoBoxStyle = 'info' | 'warn' | 'error'

interface Props {
  title: React.ReactNode
  children: React.ReactNode
  style?: InfoBoxStyle
}

export function InfoBox (props: Props) {
  const boxStyle = props.style || 'info'
  return (
    <style.root className={boxStyle}>
      <style.icon>
        <InfoBoxIcon style={boxStyle} />
      </style.icon>
      <style.content>
        <style.title>
          {props.title}
        </style.title>
        <style.text>
          {props.children}
        </style.text>
      </style.content>
    </style.root>
  )
}
