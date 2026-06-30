/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { CustomWidget } from './custom_widget'
import { useCustomWidgets } from '../../state/custom_widgets_store'

import { style } from './custom_widgets.style'

// Custom widgets are half the width of a regular widget, so we pack two of them
// into each regular-width grid column. Each column is rendered as a direct child
// of the `.widget-container` grid, so adding widgets adds columns horizontally
// and shrinks the regular widgets when space runs out.
export function CustomWidgets() {
  const widgets = useCustomWidgets()
  if (widgets.length === 0) {
    return null
  }

  const columns: (typeof widgets)[] = []
  for (let i = 0; i < widgets.length; i += 2) {
    columns.push(widgets.slice(i, i + 2))
  }

  return (
    <>
      {columns.map((column, index) => (
        <div
          key={index}
          className='custom-widget-column'
          data-css-scope={style.scope}
        >
          {column.map((widget) => (
            <CustomWidget
              key={widget.id}
              widget={widget}
            />
          ))}
        </div>
      ))}
    </>
  )
}
