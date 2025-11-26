/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import Icon from '@brave/leo/react/icon'

import { style } from './widget_menu.style'

export function WidgetMenu(props: React.PropsWithChildren) {
  return (
    <div data-css-scope={style.scope}>
      <ButtonMenu placement='bottom-end'>
        <Button
          fab
          kind='plain-faint'
          size='small'
          slot='anchor-content'
        >
          <Icon name='more-vertical' />
        </Button>
        {props.children}
      </ButtonMenu>
    </div>
  )
}
