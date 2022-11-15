// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { AccountButtonOptionsObjectType } from '../../../constants/types'

// Styled Components
import {
  OvalButton,
  OvalButtonText,
  Icon
} from './style'

// Utils
import { getLocale } from '../../../../common/locale'

interface Props {
  option: AccountButtonOptionsObjectType
  hideIcon?: boolean
  onClick: () => void
}

export const AccountListItemOptionButton = (props: Props) => {
  const {
    onClick,
    hideIcon,
    option
  } = props
  return (
    <OvalButton onClick={onClick}>
      {!hideIcon &&
        <Icon icon={option.icon} />
      }
      <OvalButtonText>{getLocale(option.name)}</OvalButtonText>
    </OvalButton>
  )
}
export default AccountListItemOptionButton
