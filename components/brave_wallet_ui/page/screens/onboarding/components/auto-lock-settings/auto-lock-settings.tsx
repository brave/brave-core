// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { AutoLockOption } from '../../../../../constants/types'

// styles
import { Row } from '../../../../../components/shared/style'
import {
  DurationDropdown,
  LockIcon,
  LockIconContainer,
  SettingDescription,
  OptionLabel
} from './auto-lock-settings.style'
import { getLocale } from '../../../../../../common/locale'

interface Props {
  options: AutoLockOption[]
  value: number
  onChange: (value: number) => void
}

export const AutoLockSettings = ({ options, value, onChange }: Props) => {
  const selectedOption = React.useMemo(() => {
    return options.find((option) => option.value === value) || options[0]
  }, [options, value])

  return (
    <Row
      justifyContent='justify-between'
      alignItems='center'
      padding='0 23px 0'
      gap='8px'
      width='100%'
    >
      <LockIconContainer>
        <LockIcon />
      </LockIconContainer>
      <SettingDescription>
        {getLocale('braveWalletAutoLockTitle')}
      </SettingDescription>
      <DurationDropdown
        mode='filled'
        value={selectedOption?.value.toString()}
        onChange={(e) => onChange(parseInt(e.detail.value, 10))}
      >
        <div slot='value'>{selectedOption?.label}</div>
        {options.map((option) => (
          <leo-option
            key={option.value}
            value={option.value.toString()}
          >
            <OptionLabel>{option.label}</OptionLabel>
          </leo-option>
        ))}
      </DurationDropdown>
    </Row>
  )
}
