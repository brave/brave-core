// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useLocation } from 'react-router-dom'

// Types
import { WalletRoutes } from '../../../constants/types'

// Selectors
import { WalletSelectors } from '../../../common/selectors'

// Hooks
import { useSafeWalletSelector } from '../../../common/hooks/use-safe-selector'

// Options
import { AllNavOptions } from '../../../options/nav-options'

// Components
import { WalletNav } from '../wallet-nav/wallet-nav'
import {
  FeatureRequestButton
} from '../../shared/feature-request-button/feature-request-button'
import {
  TabHeader
} from '../../../page/screens/shared-screen-components/tab-header/tab-header'

// Styles
import {
  Wrapper,
  LayoutCardWrapper,
  ContainerCard,
  StaticBackground,
  BackgroundGradientWrapper,
  BackgroundGradientTopLayer,
  BackgroundGradientMiddleLayer,
  BackgroundGradientBottomLayer,
  BlockForHeight
} from './wallet-page-wrapper.style'

export interface Props {
  wrapContentInBox?: boolean
  cardWidth?: number
  cardOverflow?: 'auto' | 'hidden' | 'visible'
  noPadding?: boolean
  hideBackground?: boolean
  children?: React.ReactNode
}

export const WalletPageWrapper = (props: Props) => {
  const {
    children,
    cardWidth,
    cardOverflow,
    noPadding,
    wrapContentInBox,
    hideBackground
  } = props

  // Routing
  const { pathname: walletLocation } = useLocation()

  // Wallet Selectors (safe)
  const isWalletCreated = useSafeWalletSelector(WalletSelectors.isWalletCreated)
  const isWalletLocked = useSafeWalletSelector(WalletSelectors.isWalletLocked)

  // Computed
  const showNavigationAndHeader =
    isWalletCreated && !isWalletLocked &&
    (
      walletLocation.includes(WalletRoutes.Portfolio) ||
      walletLocation.includes(WalletRoutes.Accounts) ||
      walletLocation.includes(WalletRoutes.Market) ||
      walletLocation.includes(WalletRoutes.Activity) ||
      walletLocation.includes(WalletRoutes.Nfts) ||
      walletLocation.includes(WalletRoutes.Send) ||
      walletLocation.includes(WalletRoutes.Swap) ||
      walletLocation.includes(WalletRoutes.FundWalletPageStart) ||
      walletLocation.includes(WalletRoutes.DepositFundsPageStart)
    )

  const headerTitle = AllNavOptions.find((option) =>
    walletLocation.includes(option.route))?.name ?? ''

  return (
    <>
      <StaticBackground />
      {!hideBackground &&
        <BackgroundGradientWrapper>
          <BackgroundGradientTopLayer />
          <BackgroundGradientMiddleLayer />
          <BackgroundGradientBottomLayer />
        </BackgroundGradientWrapper>
      }
      <Wrapper noPadding={noPadding}>
        {showNavigationAndHeader && walletLocation !== WalletRoutes.Swap &&
          <TabHeader title={headerTitle} />
        }
        {showNavigationAndHeader &&
          <WalletNav isSwap={walletLocation === WalletRoutes.Swap} />
        }
        {!isWalletLocked &&
          <FeatureRequestButton />
        }
        <BlockForHeight />
        {wrapContentInBox ? (
          <LayoutCardWrapper
            maxWidth={cardWidth}
          >
            <ContainerCard
              cardOverflow={cardOverflow}
            >
              {children}
            </ContainerCard>
          </LayoutCardWrapper>
        ) : (
          children
        )}
      </Wrapper>
    </>
  )
}

export default WalletPageWrapper
