// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Types
import { BraveWallet } from '../../../../constants/types'

// Queries
import {
  useGetVisibleNetworksQuery //
} from '../../../../common/slices/api.slice'

// Utils
import { getLocale } from '../../../../../common/locale'
import {
  getNetworkId //
} from '../../../../common/slices/entities/network.entity'

// Components
import { PopupModal } from '../../popup-modals/index'
import {
  CreateAccountIcon //
} from '../../../shared/create-account-icon/create-account-icon'
import { NetworkButton } from './network_button'

// Styles
import {
  AccountInfoRow,
  StyledWrapper,
  AddressText
} from './view_on_block_explorer_modal.style'
import { Column, Text, Row, ScrollableColumn } from '../../../shared/style'

interface Props {
  account: BraveWallet.AccountInfo
  onClose: () => void
}

export const ViewOnBlockExplorerModal = (props: Props) => {
  const { account, onClose } = props

  // Queries
  const { data: visibleNetworks = [] } = useGetVisibleNetworksQuery()

  // Memos
  const networksByAccountCoinType = React.useMemo(() => {
    return visibleNetworks.filter(
      (network) => network.coin === account.accountId.coin
    )
  }, [visibleNetworks, account])

  return (
    <PopupModal
      title={getLocale('braveWalletTransactionExplorer')}
      onClose={onClose}
      width='520px'
    >
      <StyledWrapper
        fullWidth={true}
        fullHeight={true}
        alignItems='flex-start'
        justifyContent='flex-start'
        padding='0px 16px 16px 16px'
      >
        <AccountInfoRow
          justifyContent='flex-start'
          marginBottom={16}
        >
          <CreateAccountIcon
            account={account}
            size='huge'
            marginRight={16}
          />
          <Column alignItems='flex-start'>
            <Text
              isBold={true}
              textColor='primary'
              textSize='14px'
            >
              {account.name}
            </Text>
            <AddressText
              isBold={false}
              textColor='secondary'
              textSize='12px'
              textAlign='left'
            >
              {account.address}
            </AddressText>
          </Column>
        </AccountInfoRow>
        <Row
          padding='16px'
          justifyContent='flex-start'
        >
          <Text
            isBold={true}
            textColor='primary'
            textSize='14px'
          >
            {getLocale('braveWalletViewAddressOn')}
          </Text>
        </Row>
        <ScrollableColumn>
          {networksByAccountCoinType.map((network) => (
            <NetworkButton
              key={getNetworkId(network)}
              network={network}
              address={account.accountId.address}
            />
          ))}
        </ScrollableColumn>
      </StyledWrapper>
    </PopupModal>
  )
}
