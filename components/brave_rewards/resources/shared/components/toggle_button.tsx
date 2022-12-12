/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import * as style from './toggle_button.style'

interface Props {
  checked: boolean
  onChange: (checked: boolean) => void
}

export function ToggleButton (props: Props) {
  function toggleChecked () {
    props.onChange(!props.checked)
  }

  return (
    <style.root>
      <button
        onClick={toggleChecked}
        className={props.checked ? 'checked' : ''}
      >
        <style.handle />
      </button>
    </style.root>
  )
}
