// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Alert from '@brave/leo/react/alert'
import * as leo from '@brave/leo/tokens/css/variables'

// utils
import { getLocale } from '../../../../common/locale'

// components
import { BottomSheet } from '../../shared/bottom_sheet/bottom_sheet'

// styles
import { Column, LeoSquaredButton } from '../../shared/style'
import {
  AlertTextContainer,
  FullWidthChildrenColumn,
  CollapseTitle,
  TitleText,
  alertItemGap,
  CollapseTitleRow,
  CollapseIcon
} from './evm_message_simulation_not_supported_sheet.styles'

export const EvmMessageSimulationNotSupportedSheet = () => {
  // state
  const [showSheet, setShowSheet] = React.useState(true)
  const [isMessageExpanded, setIsMessageExpanded] = React.useState(false)

  // render
  return (
    <BottomSheet isOpen={showSheet}>
      <TitleText>
        {getLocale('braveWalletEvmMessageScanningNotSupported')}
      </TitleText>
      <FullWidthChildrenColumn
        gap={'16px'}
        padding={'0px 16px 16px 16px'}
      >
        <Alert type='info'>
          <div slot='icon'></div>

          <Column
            justifyContent='center'
            alignItems='center'
            gap={alertItemGap}
          >
            <AlertTextContainer>
              {getLocale('braveWalletTransactionSimulationOptedInNotice')}
            </AlertTextContainer>

            <Column width='100%'>
              <CollapseTitleRow
                onClick={() => setIsMessageExpanded((prev) => !prev)}
              >
                <CollapseTitle>
                  {getLocale('braveWalletWhatIsMessageScanning')}
                  <CollapseIcon
                    name={isMessageExpanded ? 'carat-up' : 'carat-down'}
                  />
                </CollapseTitle>
              </CollapseTitleRow>

              {isMessageExpanded && (
                <Column gap={leo.spacing.m}>
                  <AlertTextContainer>
                    {getLocale(
                      'braveWalletEvmMessageScanningFeatureSafetyExplanation'
                    )}
                  </AlertTextContainer>
                  <AlertTextContainer>
                    {getLocale(
                      'braveWalletEvmMessageScanningFeatureAccuracyExplanation'
                    )}
                  </AlertTextContainer>
                </Column>
              )}
            </Column>
          </Column>
        </Alert>
        <LeoSquaredButton
          onClick={() => {
            setShowSheet(false)
          }}
        >
          {getLocale('braveWalletButtonClose')}
        </LeoSquaredButton>
      </FullWidthChildrenColumn>
    </BottomSheet>
  )
}

export default EvmMessageSimulationNotSupportedSheet
