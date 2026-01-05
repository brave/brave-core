// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Selectors
import { WalletSelectors, UISelectors } from '../../../common/selectors'

// Hooks
import {
  useSafeWalletSelector,
  useSafeUISelector,
} from '../../../common/hooks/use-safe-selector'

// Components
import { WalletNav } from '../wallet-nav/wallet-nav'
import {
  FeatureRequestButton, //
} from '../../shared/feature-request-button/feature-request-button'
import {
  TabHeader, //
} from '../../../page/screens/shared-screen-components/tab-header/tab-header'

// Styles
import {
  Wrapper,
  LayoutCardWrapper,
  ContainerCard,
  StaticBackground,
  BlockForHeight,
  FeatureRequestButtonWrapper,
  CardHeaderWrapper,
  CardHeader,
  CardHeaderShadow,
  CardHeaderContentWrapper,
  PortfolioBackgroundWatermark,
  ConnectionBackgroundWatermark,
  ConnectionBackgroundColor,
} from './wallet-page-wrapper.style'

export interface Props {
  wrapContentInBox?: boolean
  noPadding?: boolean
  noCardPadding?: boolean
  hideNav?: boolean
  hideHeader?: boolean
  hideHeaderMenu?: boolean
  cardHeader?: JSX.Element | undefined | null
  noMinCardHeight?: boolean
  noBorderRadius?: boolean
  useDarkBackground?: boolean
  useCardInPanel?: boolean
  useFullHeight?: boolean
  isPortfolio?: boolean
  isConnection?: boolean
  children?: React.ReactNode
}

export const WalletPageWrapper = (props: Props) => {
  const {
    children,
    noPadding,
    noCardPadding,
    wrapContentInBox,
    cardHeader,
    hideNav,
    hideHeader,
    hideHeaderMenu,
    noMinCardHeight,
    noBorderRadius,
    useDarkBackground,
    useCardInPanel,
    useFullHeight,
    isPortfolio,
    isConnection,
  } = props

  // Wallet Selectors (safe)
  const isWalletCreated = useSafeWalletSelector(WalletSelectors.isWalletCreated)
  const isWalletLocked = useSafeWalletSelector(WalletSelectors.isWalletLocked)
  const isPanel = useSafeUISelector(UISelectors.isPanel)
  const isMobile = useSafeUISelector(UISelectors.isMobile)
  const isMobileOrPanel = isMobile || isPanel

  // State
  const [headerShadowOpacity, setHeaderShadowOpacity] =
    React.useState<number>(0)
  const [headerBackgroundOpacity, setHeaderBackgroundOpacity] =
    React.useState<number>(0)
  const [headerHeight, setHeaderHeight] = React.useState<number>(0)

  // Computed
  const shouldUsePanelCard = useCardInPanel && isMobileOrPanel

  // Refs
  let scrollRef = React.useRef<HTMLDivElement | null>(null)
  const headerRef = React.createRef<HTMLDivElement>()

  React.useEffect(() => {
    // Keeps track of the Header height to update
    // the card top position and headers shadow.
    if (cardHeader) {
      setHeaderHeight(headerRef?.current?.clientHeight ?? 0)
    }
  }, [cardHeader, headerRef])

  const onScroll = React.useCallback(() => {
    const scrollPosition = scrollRef.current
    if (scrollPosition !== null) {
      const { scrollTop } = scrollPosition

      // Assures that shadowOpacity, dividerOpacity and backgroundOpacity are
      // the expected values when scrollTop is 0, since some values
      // may not get calculated when scrolling fast.
      if (scrollTop === 0) {
        setHeaderShadowOpacity(0)
        setHeaderBackgroundOpacity(0)
        return
      }

      // Calculates opacity values for the first 64 scroll positions.
      if (scrollTop <= 64) {
        // Increases shadowOpacity by 0.00125 until it reaches
        // desired opacity of 0.08, or will decrease shadowOpacity by
        // 0.00125 until it reaches desired opacity of 0.
        // example: 0.00125 * 64 = 0.08
        setHeaderShadowOpacity((scrollTop / 8) * 0.01)

        // Increases backgroundOpacity by 0.015625 until it reaches
        // desired opacity of 1, or will decrease backgroundOpacity by
        // 0.015625 until it reaches desired opacity of 0.
        // example: 0.015625 * 64 = 1
        setHeaderBackgroundOpacity((100 / 64) * scrollTop * 0.01)
        return
      }

      // Assures that shadowOpacity and backgroundOpacity are
      // the expected values when scrollTop is greater than 64,
      // since some values may not get calculated when scrolling fast.
      setHeaderShadowOpacity(0.08)
      setHeaderBackgroundOpacity(1)
    }
  }, [scrollRef])

  return (
    <>
      <StaticBackground />
      <Wrapper
        noPadding={noPadding}
        noTopPosition={isMobileOrPanel}
      >
        {isMobileOrPanel && isPortfolio && <PortfolioBackgroundWatermark />}
        {isPanel && isConnection && (
          <>
            <ConnectionBackgroundColor />
            <ConnectionBackgroundWatermark />
          </>
        )}
        {isWalletCreated && !hideHeader && !isPanel && !isMobile && (
          <TabHeader hideHeaderMenu={hideHeaderMenu} />
        )}
        {isWalletCreated && !isWalletLocked && !hideNav && <WalletNav />}
        {!isWalletLocked && (
          <FeatureRequestButtonWrapper>
            <FeatureRequestButton />
          </FeatureRequestButtonWrapper>
        )}
        <BlockForHeight />

        {wrapContentInBox ? (
          <LayoutCardWrapper
            ref={scrollRef}
            onScroll={onScroll}
            hideCardHeader={!cardHeader}
            headerHeight={headerHeight}
            hideNav={hideNav}
            padding={useFullHeight ? '0px' : undefined}
          >
            {cardHeader && !isMobileOrPanel && (
              <CardHeaderWrapper>
                <CardHeaderShadow headerHeight={headerHeight} />
              </CardHeaderWrapper>
            )}

            <ContainerCard
              noPadding={noCardPadding}
              hideCardHeader={!cardHeader}
              noMinCardHeight={noMinCardHeight}
              noBorderRadius={noBorderRadius}
              useDarkBackground={useDarkBackground}
              useFullHeight={useFullHeight}
              noBackground={isMobileOrPanel && (isPortfolio || isConnection)}
              usePanelCard={shouldUsePanelCard}
            >
              {children}
            </ContainerCard>

            {cardHeader && (
              <CardHeaderWrapper
                ref={headerRef}
                isMobileOrPanel={isMobileOrPanel}
              >
                <CardHeader
                  shadowOpacity={headerShadowOpacity}
                  isMobileOrPanel={isMobileOrPanel}
                  useDarkBackground={useDarkBackground || shouldUsePanelCard}
                  backgroundOpacity={headerBackgroundOpacity}
                >
                  <CardHeaderContentWrapper>
                    {cardHeader}
                  </CardHeaderContentWrapper>
                </CardHeader>
              </CardHeaderWrapper>
            )}
          </LayoutCardWrapper>
        ) : (
          children
        )}
      </Wrapper>
    </>
  )
}

export default WalletPageWrapper
