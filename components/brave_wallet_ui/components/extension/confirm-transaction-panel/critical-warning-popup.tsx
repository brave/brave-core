// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../common/locale'

// Components
import { FullPanelPopup } from '../full_panel_popup/full_panel_popup'

// Styles
import { Column, LeoSquaredButton } from '../../shared/style'
import {
  LargeWarningCircleIcon,
  FullWidthChildrenColumn,
  WarningButtonText,
  CriticalWarningTitle,
  CriticalWarningDescription
} from './critical-warning-popup.styles'

interface Props {
  onProceed: () => void
  onCancel: (() => void) | (() => Promise<void>)
}

export const CriticalWarningPopup: React.FC<Props> = ({
  onProceed,
  onCancel
}) => {
  return (
    <FullPanelPopup kind='danger'>
      <Column
        fullHeight
        fullWidth
        gap={'8px'}
        justifyContent='space-between'
      >
        <Column
          fullWidth
          fullHeight
          gap={'8px'}
          padding={'40px'}
          alignSelf='center'
        >
          <LargeWarningCircleIcon />

          <CriticalWarningTitle>
            {getLocale('braveWalletRiskOfLossAction')}
          </CriticalWarningTitle>

          <CriticalWarningDescription>
            {getLocale('braveWalletUntrustedRequestWarning')}
          </CriticalWarningDescription>
        </Column>

        <FullWidthChildrenColumn
          fullWidth
          padding={'16px'}
          gap={'16px'}
        >
          <LeoSquaredButton
            kind='outline'
            onClick={onProceed}
          >
            <WarningButtonText>
              {getLocale('braveWalletProceedButton')}
            </WarningButtonText>
          </LeoSquaredButton>
          <LeoSquaredButton
            kind='filled'
            onClick={onCancel}
          >
            {getLocale('braveWalletButtonCancel')}
          </LeoSquaredButton>
        </FullWidthChildrenColumn>
      </Column>
    </FullPanelPopup>
  )
}
