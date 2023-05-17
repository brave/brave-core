// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Redux
import {
  useDispatch
} from 'react-redux'
import {
  WalletActions
} from '../../../../../../common/actions'

// Utils
import {
  getLocale
} from '../../../../../../../common/locale'

// Assets
import HelpIcon from '~/assets/info-icon.svg'
import CloseIcon from '~/assets/close-icon.svg'

// Hiding Portfolio Section until we support it.
// import PortfolioIcon from '~/assets/portfolio-icon.svg'

// Selectors
import {
  WalletSelectors
} from '../../../../../../common/selectors'
import {
  useUnsafeWalletSelector
} from '../../../../../../common/hooks/use-safe-selector'

import {
  useGetSelectedChainQuery
} from '../../../../../../common/slices/api.slice'

// Types
import { WalletAccountType } from '../../../../../../constants/types'
// import { RefreshBlockchainStateParams } from '~/constants/types'

// Components
import { AccountListItemButton } from './account-list-item-button'
import { AccountModalButton } from './account-modal-button'

// Styled Components
import { ModalBox, Title } from './account-modal.style'
import {
  Row,
  Column,
  VerticalDivider,
  IconButton,
  ShownResponsiveRow
} from '../../shared-swap.styles'

interface Props {
  onHideModal: () => void
  // refreshBlockchainState: (
  //   overrides: Partial<RefreshBlockchainStateParams>
  // ) => Promise<void>
}

export const AccountModal = (props: Props) => {
  const { onHideModal } = props
  // Redux
  const dispatch = useDispatch()

  // Queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()

  // Selectors
  const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)

  // Memos
  const networkAccounts = React.useMemo(() => {
    return accounts.filter(account => account.coin === selectedNetwork?.coin)
  }, [accounts, selectedNetwork])

  // Methods
  const onSelectAccount = React.useCallback(
    (account: WalletAccountType) => {
      dispatch(WalletActions.setSelectedAccount(account))
      onHideModal()
      // await refreshBlockchainState({ account })
    },
    [
      onHideModal
      // refreshBlockchainState
    ]
  )

  const onClickHelpCenter = React.useCallback(() => {
    window.open(
      'https://support.brave.com/hc/en-us/articles/8155407080845-Brave-Swaps-FAQ'
    )
  }, [])

  return (
    <ModalBox>
      {/* Hiding Porfolio Section until we support it */}
      {/* <Column
        columnWidth='full'
        verticalPadding={16}
        horizontalPadding={16}
        horizontalAlign='flex-start'
        verticalAlign='flex-start'
      >
        <Text textSize='12px' textColor='text02' isBold={false}>
          {getLocale('braveSwapPortfolioBalance')}
        </Text>
        <VerticalSpacer size={10} />
        <Text textSize='16px' textColor='text01' isBold={true}>
          $10,731.32
        </Text>
      </Column>
      <VerticalDivider /> */}
      <Column
        columnWidth='full'
        verticalPadding={12}
        horizontalPadding={6}
        horizontalAlign='flex-start'
        verticalAlign='flex-start'
      >
        <Row verticalPaddingResponsive={8} horizontalPadding={10} rowWidth='full' marginBottom={4}>
          <Row>
            <Title textSize='12px' responsiveTextSize='16px' textColor='text02' isBold={false}>
              {getLocale('braveSwapAccounts')}
            </Title>
          </Row>
          <ShownResponsiveRow maxWidth={570}>
            <IconButton icon={CloseIcon} onClick={onHideModal} size={20} />
          </ShownResponsiveRow>
        </Row>
        {networkAccounts.map((account) => (
          <AccountListItemButton
            key={account.address}
            address={account.address}
            name={account.name}
            onClick={() => onSelectAccount(account)}
          />
        ))}
      </Column>
      <VerticalDivider />
      <Column
        columnWidth='full'
        verticalPadding={4}
        horizontalPadding={16}
        horizontalAlign='flex-start'
        verticalAlign='flex-start'
      >
        {/* Hiding Porfolio Section until we support it */}
        {/* <AccountModalButton
          text={getLocale('braveSwapMyPortfolio')}
          icon={PortfolioIcon}
          onClick={onClickViewPortfolio}
        /> */}
        <AccountModalButton
          text={getLocale('braveSwapHelpCenter')}
          icon={HelpIcon}
          onClick={onClickHelpCenter}
        />
      </Column>
    </ModalBox>
  )
}
