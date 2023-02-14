/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import * as style from './tooltip.style'

interface Props {
  children: React.ReactNode
}

export function Tooltip (props: Props) {
  return (
    <style.root>
      <style.content>
        {props.children}
        <style.arrow />
      </style.content>
    </style.root>
  )
}
