/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { CustomWidget } from './custom_widget'
import { useCustomWidgets } from '../../state/custom_widgets_store'

import { style } from './custom_widgets.style'

export function CustomWidgets() {
  const widgets = useCustomWidgets()
  if (widgets.length === 0) {
    return null
  }
  return (
    <div data-css-scope={style.scope}>
      {widgets.map((widget) => (
        <CustomWidget
          key={widget.id}
          widget={widget}
        />
      ))}
    </div>
  )
}
