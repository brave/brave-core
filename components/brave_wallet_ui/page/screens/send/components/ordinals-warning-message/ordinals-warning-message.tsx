// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../../../common/locale'

// Components
import { Checkbox } from 'brave-ui/components'

// Shared Styles
import { Column, Text } from '../../shared.styles'

// Styles
import {
  CheckboxWrapper,
  StyledRow,
  WarningIcon
} from './ordinals-warning-message.style'

interface Props {
  acknowledged: boolean
  onChange: (acknowledged: boolean) => void
}

export const OrdinalsWarningMessage = ({ acknowledged, onChange }: Props) => {
  return (
    <StyledRow
      verticalAlign='flex-start'
      horizontalAlign='flex-start'
    >
      <WarningIcon name='warning-triangle-filled' />
      <Column horizontalAlign='flex-start'>
        <Text
          textSize='12px'
          textColor='warning'
          textAlign='left'
          isBold={false}
        >
          {getLocale('braveWalletOrdinalsWarningMessage')}
        </Text>
        <CheckboxWrapper>
          <Checkbox
            value={{ selected: acknowledged }}
            onChange={(_, selected) => onChange(selected)}
          >
            <div data-key='selected'>
              <Text
                textSize='12px'
                textColor='warning'
                textAlign='left'
                isBold={false}
              >
                {getLocale('braveWalletOrdinalsWarningLabel')}
              </Text>
            </div>
          </Checkbox>
        </CheckboxWrapper>
      </Column>
    </StyledRow>
  )
}
