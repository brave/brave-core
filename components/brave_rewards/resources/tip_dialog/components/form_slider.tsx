/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import * as style from './form_slider.style'

interface Props {
  activeForm: number
  children: React.ReactNode[]
}

export function FormSlider (props: Props) {
  const formCount = props.children.length
  const activeForm = props.activeForm > 0
    ? Math.floor(props.activeForm)
    : 0

  const cssProps: React.CSSProperties = {
    width: (formCount * 100) + '%',
    marginLeft: (activeForm * -100) + '%'
  }

  return (
    <style.root>
      <style.track style={cssProps}>
        {
          props.children.map((child, index) => {
            const role = index === activeForm ? 'active-form' : ''
            return (
              <div key={index} data-test-role={role}>
                {child}
              </div>
            )
          })
        }
      </style.track>
    </style.root>
  )
}
