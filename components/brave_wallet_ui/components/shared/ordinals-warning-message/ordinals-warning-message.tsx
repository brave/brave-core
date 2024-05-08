// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../common/locale'

// Components
import Checkbox from '@brave/leo/react/checkbox'

// Shared Styles
import { Column, Text } from '../style'

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
    <StyledRow>
      <WarningIcon />
      <Column alignItems='flex-start'>
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
            checked={acknowledged}
            onChange={() => onChange(!acknowledged)}
          >
            <Text
              textSize='12px'
              textColor='warning'
              textAlign='left'
              isBold={false}
            >
              {getLocale('braveWalletUserUnderstandsLabel')}
            </Text>
          </Checkbox>
        </CheckboxWrapper>
      </Column>
    </StyledRow>
  )
}
