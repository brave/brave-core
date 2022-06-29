// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { AccountButtonOptionsObjectType } from '../../../constants/types'

// Styled Components
import {
  OvalButton,
  OvalButtonText,
  Icon
} from './style'

export interface Props {
  option: AccountButtonOptionsObjectType
  onClick: () => void
}

const AccountListItemButton = (props: Props) => {
  const {
    onClick,
    option
  } = props
  return (
    <OvalButton onClick={onClick}>
      <Icon icon={option.icon} />
      <OvalButtonText>{option.name}</OvalButtonText>
    </OvalButton>
  )
}
export default AccountListItemButton
