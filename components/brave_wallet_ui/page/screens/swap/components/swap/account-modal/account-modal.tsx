// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import {
  getLocale
} from '../../../../../../../common/locale'

import {
  useGetAccountInfosRegistryQuery,
  useGetSelectedChainQuery,
  useSetSelectedAccountMutation
} from '../../../../../../common/slices/api.slice'
import {
  AccountInfoEntity,
  accountInfoEntityAdaptorInitialState
} from '../../../../../../common/slices/entities/account-info.entity'

import {
  getEntitiesListFromEntityState
} from '../../../../../../utils/entities.utils'

// Components
import { AccountListItemButton } from './account-list-item-button'

// Styled Components
import { ModalBox, Title } from './account-modal.style'
import {
  Row,
  Column,
  IconButton,
  Icon,
  ShownResponsiveRow
} from '../../shared-swap.styles'


interface Props {
  onHideModal: () => void
}

export const AccountModal = (props: Props) => {
  const { onHideModal } = props

  // Queries / mutations
  const { data: selectedNetwork } = useGetSelectedChainQuery()
  const [setSelectedAccount] = useSetSelectedAccountMutation()
  const { data: accountInfosRegistry = accountInfoEntityAdaptorInitialState } =
    useGetAccountInfosRegistryQuery(undefined)
  const accounts = getEntitiesListFromEntityState(accountInfosRegistry)

  // Memos
  const networkAccounts = React.useMemo(() => {
    return accounts.filter(account => account.accountId.coin === selectedNetwork?.coin)
  }, [accounts, selectedNetwork])

  // Methods
  const onSelectAccount = React.useCallback(
    async (account: AccountInfoEntity) => {
      await setSelectedAccount(account.accountId)
      onHideModal()
    },
    [onHideModal]
  )

  return (
    <ModalBox>
      {/*
        * TODO(onyb): this should be removed since the account dropdown is
        * eventually going to be replaced with Send-like UX.
        *
        * Hiding Porfolio Section until we support/remove it
        */
      }
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
        <Row
          verticalPaddingResponsive={8}
          horizontalPadding={10}
          rowWidth='full'
          marginBottom={4}
        >
          <Row>
            <Title
              textSize='12px'
              responsiveTextSize='16px'
              textColor='text02'
              isBold={false}
            >
              {getLocale('braveSwapAccounts')}
            </Title>
          </Row>
          <ShownResponsiveRow maxWidth={570}>
            <IconButton onClick={onHideModal}>
              <Icon size={28} name='close' />
            </IconButton>
          </ShownResponsiveRow>
        </Row>
        {networkAccounts.map((account) => (
          <AccountListItemButton
            key={account.accountId.uniqueKey}
            account={account}
            onClick={() => onSelectAccount(account)}
          />
        ))}
      </Column>
    </ModalBox>
  )
}
