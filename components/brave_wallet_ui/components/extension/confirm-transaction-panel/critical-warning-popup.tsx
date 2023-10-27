// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'

// Utils
import { getLocale } from '../../../../common/locale'

// Components
import { FullPanelPopup } from '../full_panel_popup/full_panel_popup'

// Styles
import { Column, Text } from '../../shared/style'
import {
  LargeWarningCircleIcon,
  FullWidthChildrenColumn,
  WarningButtonText
} from './style'

interface Props {
  onProceed: () => void
  onCancel: (() => void) | (() => Promise<void>)
}

export const CriticalWarningPopup: React.FC<Props> = ({
  onProceed,
  onCancel
}) => {
  return (
    <FullPanelPopup onClose={onProceed} kind='danger'>
      <Column fullHeight fullWidth gap={'8px'} justifyContent='space-between'>
        <Column fullWidth gap={'8px'} padding={'40px 40px 40px 40px'}>
          <LargeWarningCircleIcon />

          <Text isBold textSize='14px' lineHeight='24px'>
            {getLocale('braveWalletRiskOfLossAction')}
          </Text>

          <Text textSize='12px' lineHeight='18px'>
            {getLocale('braveWalletUntrustedRequestWarning')}
          </Text>
        </Column>

        <FullWidthChildrenColumn fullWidth padding={'16px'} gap={'16px'}>
          <Button kind='outline' onClick={onProceed}>
            <WarningButtonText>
              {getLocale('braveWalletProceedButton')}
            </WarningButtonText>
          </Button>
          <Button kind='filled' onClick={onCancel}>
            {getLocale('braveWalletButtonCancel')}
          </Button>
        </FullWidthChildrenColumn>
      </Column>
    </FullPanelPopup>
  )
}
