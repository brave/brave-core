// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// components
import NewPasswordInput, {
  NewPasswordValues
} from '../../../../../components/shared/password-input/new-password-input'
import { AutoLockSettings } from '../../components/auto-lock-settings/auto-lock-settings'

// styles
import { VerticalSpace } from '../../../../../components/shared/style'
import { AutoLockOption } from '../../../../../constants/types'

interface Props {
  autoLockDuration: number
  autoLockOptions: AutoLockOption[]
  onSubmit: (values: NewPasswordValues) => void
  onPasswordChange: (values: NewPasswordValues) => void
  onAutoLockDurationChange: (autoLockDuration: number) => void
}

export function CreatePassword({
  autoLockDuration,
  autoLockOptions,
  onSubmit,
  onPasswordChange,
  onAutoLockDurationChange
}: Props) {
  return (
    <>
      <NewPasswordInput
        autoFocus={true}
        onSubmit={onSubmit}
        onChange={onPasswordChange}
      />

      <VerticalSpace space='68px' />

      <AutoLockSettings
        options={autoLockOptions}
        value={autoLockDuration}
        onChange={onAutoLockDurationChange}
      />
    </>
  )
}
