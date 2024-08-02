// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Alert from '@brave/leo/react/alert'

// utils
import { getLocale } from '../../../../common/locale'
import { openTab } from '../../../utils/routes-utils'

// components
import { BottomSheet } from '../../shared/bottom_sheet/bottom_sheet'

// styles
import { Column, LeoSquaredButton } from '../../shared/style'
import {
  AlertTextContainer,
  FullWidthChildrenColumn,
  SeeAvailableNetworksLink, //
  TitleText,
  alertItemGap
} from './transaction_simulation_not_supported_sheet.styles'

const openSupportedNetworksList = () => {
  openTab(
    'https://github.com/brave/brave-browser/wiki/' +
      'Transaction-Simulation#current-supported-networks'
  )
}

export const TransactionSimulationNotSupportedSheet = () => {
  // state
  const [showSheet, setShowSheet] = React.useState(true)

  // render
  return (
    <BottomSheet isOpen={showSheet}>
      <TitleText>
        {getLocale('braveWalletTransactionSimulationNotAvailableForNetwork')}
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

            <SeeAvailableNetworksLink onClick={openSupportedNetworksList}>
              {getLocale('braveWalletSeeAvailableNetworks')}
            </SeeAvailableNetworksLink>
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

export default TransactionSimulationNotSupportedSheet
