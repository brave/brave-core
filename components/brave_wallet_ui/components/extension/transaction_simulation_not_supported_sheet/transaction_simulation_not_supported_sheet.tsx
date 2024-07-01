// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Checkbox from '@brave/leo/react/checkbox'
import Alert from '@brave/leo/react/alert'

// types
import { BraveWallet } from '../../../constants/types'

// utils
import { useSyncedLocalStorage } from '../../../common/hooks/use_local_storage'
import { getNetworkId } from '../../../common/slices/entities/network.entity'
import { getLocale } from '../../../../common/locale'
import { openTab } from '../../../utils/routes-utils'
import { LOCAL_STORAGE_KEYS } from '../../../common/constants/local-storage-keys'

// components
import { BottomSheet } from '../../shared/bottom_sheet/bottom_sheet'

// styles
import { Column, LeoSquaredButton } from '../../shared/style'
import {
  AlertTextContainer,
  CheckboxText,
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

export const TransactionSimulationNotSupportedSheet = ({
  network
}: {
  network: Pick<BraveWallet.NetworkInfo, 'coin' | 'chainId'>
}) => {
  // computed from props
  const networkEntityId = getNetworkId(network)

  // local storage
  const [doNotShowAgainNetworks, setDoNotShowAgainNetworks] =
    useSyncedLocalStorage<Record<string, boolean>>(
      LOCAL_STORAGE_KEYS.DO_NOT_SHOW_TX_PREVIEW_NOT_SUPPORTED_MSG_FOR_CHAINS,
      {}
    )
  const doNotShowAgain = doNotShowAgainNetworks[networkEntityId] ?? false

  // state
  const [showSheet, setShowSheet] = React.useState(true)
  const [isDoNotShowAgainChecked, setIsDoNotShowAgainChecked] =
    React.useState(doNotShowAgain)

  // render
  if (doNotShowAgain) {
    return null
  }

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
        <Checkbox
          checked={isDoNotShowAgainChecked}
          onChange={({ checked }) => {
            setIsDoNotShowAgainChecked(checked)
          }}
        >
          <CheckboxText>
            {getLocale('braveWalletDoNotShowThisMessageAgainForNetwork')}
          </CheckboxText>
        </Checkbox>
        <LeoSquaredButton
          onClick={() => {
            setDoNotShowAgainNetworks((prev) => {
              return {
                ...prev,
                [networkEntityId]: isDoNotShowAgainChecked
              }
            })
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
