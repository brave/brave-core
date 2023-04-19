/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useLocaleContext } from '../lib/locale_strings'
import { Tooltip } from './tooltip'
import { ToggleButton } from '../../shared/components/toggle_button'
import { InfoIcon } from './icons/info_icon'

import * as style from './monthly_toggle.style'

interface Props {
  checked: boolean
  onChange: (checked: boolean) => void
}

export function MonthlyToggle (props: Props) {
  const { getString } = useLocaleContext()
  return (
    <style.root data-test-id='monthly-toggle'>
      <style.label>
        {getString('monthlyToggleLabel')}
      </style.label>
      <style.info>
        <InfoIcon />
        <div className='tooltip'>
          <Tooltip>
            {getString('monthlyTooltipText')}
          </Tooltip>
        </div>
      </style.info>
      <ToggleButton checked={props.checked} onChange={props.onChange} />
    </style.root>
  )
}
