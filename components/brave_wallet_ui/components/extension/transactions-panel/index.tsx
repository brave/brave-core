// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Types
import { BraveWallet, SerializableTransactionInfo } from '../../../constants/types'

// Utils
import { getLocale } from '../../../../common/locale'
import { PanelActions } from '../../../panel/actions'

// Hooks
import { useGetTransactionsQuery } from '../../../common/slices/api.slice'

// Components
import { TransactionsListItem } from '../transaction-list-item/index'

// Styled Components
import { ScrollContainer } from '../../../stories/style'
import { CircleIconWrapper, Column, Row, VerticalSpace } from '../../shared/style'
import {
  FillerDescriptionText,
  FillerTitleText,
  FloatAboveBottomRightCorner,
  InfoCircleIcon,
  StyledWrapper,
  TransactionsIcon
} from './style'

export interface Props {
  selectedNetwork: BraveWallet.NetworkInfo | undefined
  selectedAccount: BraveWallet.AccountId | undefined
}

export const TransactionsPanel = ({
  selectedNetwork,
  selectedAccount,
}: Props) => {
  // redux
  const dispatch = useDispatch()

  // queries
  const { data: sortedNonRejectedTransactionList = [] } =
    useGetTransactionsQuery(
      selectedAccount
        ? {
            accountId: selectedAccount ?? null,
            chainId: selectedNetwork?.chainId ?? null,
            coinType: selectedAccount.coin ?? null
          }
        : skipToken
    )

  const viewTransactionDetail = React.useCallback(
    (transaction: SerializableTransactionInfo) => {
      dispatch(PanelActions.setSelectedTransactionId(transaction.id))
      dispatch(PanelActions.navigateTo('transactionDetails'))
    },
    []
  )

  // render
  if (sortedNonRejectedTransactionList.length === 0) {
    return (
      <StyledWrapper hideScrollbar>
        <Column fullHeight padding='22px'>
          <Column>

            {/* Graphic */}
            <Row>
              <CircleIconWrapper>

                <TransactionsIcon
                  size={24}
                />

                <FloatAboveBottomRightCorner>
                  <CircleIconWrapper padding={2}>
                    <InfoCircleIcon />
                  </CircleIconWrapper>
                </FloatAboveBottomRightCorner>

              </CircleIconWrapper>
            </Row>

            <VerticalSpace space='16px' />

            <Column justifyContent='flex-start' gap='8px'>
              <FillerTitleText>
                {getLocale('braveWalletNoTransactionsYet')}
              </FillerTitleText>

              <FillerDescriptionText>
                {getLocale('braveWalletNoTransactionsYetDescription')}
              </FillerDescriptionText>
            </Column>

          </Column>
        </Column>
      </StyledWrapper>
    )
  }

  return (
    <ScrollContainer>
      <StyledWrapper>
        {sortedNonRejectedTransactionList.map((transaction) =>
          <TransactionsListItem
            key={transaction.id}
            onSelectTransaction={viewTransactionDetail}
            transaction={transaction}
          />
        )}
      </StyledWrapper>
    </ScrollContainer>
  )
}

export default TransactionsPanel
