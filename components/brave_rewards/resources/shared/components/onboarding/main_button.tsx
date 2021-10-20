/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import * as style from './main_button.style'

interface Props {
  onClick: () => void
  children: React.ReactNode
}

export function MainButton (props: Props) {
  return (
    <style.root>
      <button onClick={props.onClick} data-test-id='rewards-onboarding-main-button'>
        {props.children}
      </button>
    </style.root>
  )
}
