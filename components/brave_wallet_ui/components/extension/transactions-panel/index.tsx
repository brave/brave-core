// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import { sortTransactionByDate } from '../../../utils/tx-utils'
import { WalletSelectors } from '../../../common/selectors'

// Hooks
import { useUnsafeWalletSelector } from '../../../common/hooks/use-safe-selector'

// Components
import { TransactionsListItem } from '../'

// Styled Components
import { CircleIconWrapper, Column, Row, VerticalSpace } from '../../shared/style'
import { FillerDescriptionText, FillerTitleText, FloatAboveBottomRightCorner, InfoCircleIcon, StyledWrapper, TransactionsIcon } from './style'
import { ScrollContainer } from '../../../stories/style'
import { getLocale } from '../../../../common/locale'

export interface Props {
  selectedNetwork: BraveWallet.NetworkInfo
  selectedAccountAddress: string
  onSelectTransaction: (transaction: BraveWallet.TransactionInfo) => void
}

export const TransactionsPanel = ({
  selectedNetwork,
  selectedAccountAddress,
  onSelectTransaction
}: Props) => {
  // redux
  const transactions = useUnsafeWalletSelector(WalletSelectors.transactions)

  // memos
  const transactionList = React.useMemo(() => {
    if (selectedAccountAddress && transactions[selectedAccountAddress]) {
      return sortTransactionByDate(transactions[selectedAccountAddress], 'descending')
    } else {
      return []
    }
  }, [selectedAccountAddress, transactions])

  // render
  if (transactionList.length === 0) {
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
        {transactionList.map((transaction: BraveWallet.TransactionInfo) =>
          <TransactionsListItem
            key={transaction.id}
            onSelectTransaction={onSelectTransaction}
            selectedNetwork={selectedNetwork}
            transaction={transaction}
          />
        )}
      </StyledWrapper>
    </ScrollContainer>
  )
}

export default TransactionsPanel
