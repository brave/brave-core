// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// components
import NewPasswordInput, {
  NewPasswordValues
} from '../../../../../components/shared/password-input/new-password-input'

// styles
import { Column } from '../../../../../components/shared/style'

interface Props {
  initialPassword?: string
  onSubmit: (values: NewPasswordValues) => void
  onPasswordChange: (values: NewPasswordValues) => void
}

export function CreatePassword({
  initialPassword,
  onSubmit,
  onPasswordChange
}: Props) {
  return (
    <Column
      width='100%'
      justifyContent='center'
      alignItems='center'
    >
      <NewPasswordInput
        autoFocus={true}
        initialPassword={initialPassword}
        onSubmit={onSubmit}
        onChange={onPasswordChange}
      />
    </Column>
  )
}
