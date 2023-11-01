// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'

// Selectors
import { UISelectors } from '../../../common/selectors'

// Hooks
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'

// Options
import {
  NavOptions,
  PanelNavOptions,
  BuySendSwapDepositOptions
} from '../../../options/nav-options'

// Components
import { WalletNavButton } from './wallet-nav-button/wallet-nav-button'

// Styled Components
import {
  Wrapper,
  Section,
  PageOptionsWrapper,
  PanelOptionsWrapper
} from './wallet-nav.style'
import { WalletRoutes } from '../../../constants/types'
import { WalletActions } from '../../../common/actions'

export const WalletNav = () => {
  // redux
  const dispatch = useDispatch()
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  return (
    <Wrapper isPanel={isPanel}>
      <PanelOptionsWrapper>
        <Section>
          {PanelNavOptions.map((option) => (
            <WalletNavButton
              option={option}
              key={option.id}
            />
          ))}
        </Section>
      </PanelOptionsWrapper>

      <PageOptionsWrapper>
        <Section showBorder={true}>
          {NavOptions.map((option) => (
            <WalletNavButton
              option={option}
              key={option.id}
            />
          ))}
        </Section>
        <Section>
          {BuySendSwapDepositOptions.map((option) => (
            <WalletNavButton
              option={option}
              key={option.id}
              onClick={() => {
                if (
                  option.route === WalletRoutes.FundWalletPageStart ||
                  option.route === WalletRoutes.DepositFundsPageStart
                ) {
                  dispatch(WalletActions.selectOnRampAssetId(undefined))
                }
              }}
            />
          ))}
        </Section>
      </PageOptionsWrapper>
    </Wrapper>
  )
}

export default WalletNav
