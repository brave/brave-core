// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'

// components
import { WalletPageStory } from '../../../stories/wrappers/wallet-page-story-wrapper'
import WalletPageLayout from '../wallet-page-layout'
import WalletSubViewLayout from '../wallet-sub-view-layout'
import { PortfolioTransactionItem } from './index'

// mocks
import { mockParsedTransactionInfo } from '../../../stories/mock-data/mock-parsed-transaction-info'

export const CryptoViewWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  position: relative;
`

const PortfolioWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  margin-bottom: 20px;
`

export const _PortfolioTransactionItem: React.FC = () => {
  return (
    <WalletPageStory>
      <WalletPageLayout>
        <WalletSubViewLayout>
          <CryptoViewWrapper>
            <PortfolioWrapper>
              <PortfolioTransactionItem
                displayAccountName={true}
                transaction={mockParsedTransactionInfo}
              />
            </PortfolioWrapper>
          </CryptoViewWrapper>
        </WalletSubViewLayout>
      </WalletPageLayout>
    </WalletPageStory>
  )
}

export default _PortfolioTransactionItem
