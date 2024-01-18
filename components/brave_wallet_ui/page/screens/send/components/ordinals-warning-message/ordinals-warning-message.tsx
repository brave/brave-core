// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../../../common/locale'

// Shared Styles
import { Text } from '../../shared.styles'

// Styles
import { StyledRow, WarningIcon } from './ordinals-warning-message.style'

export const OrdinalsWarningMessage = () => {
  return (
    <StyledRow
      verticalAlign='center'
      horizontalAlign='flex-start'
    >
      <WarningIcon name='warning-triangle-filled' />
      <Text
        textSize='12px'
        textColor='warning'
        textAlign='left'
        isBold={false}
      >
        {getLocale('braveWalletOrdinalsWarningMessage')}
      </Text>
    </StyledRow>
  )
}
