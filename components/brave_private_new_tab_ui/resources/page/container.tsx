// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'
import Icon from '@brave/leo/react/icon'
import * as leo from '@brave/leo/tokens/css/variables'
import { getLocale } from '$web-common/locale'
import getPageHandlerInstance from './api/brave_page_handler'
import { loadTimeData } from '../../../common/loadTimeData'

import BackgroundView from './components/background-view'
import SearchBox from './components/search-box'
import BadgeTor from './components/badge-tor'
import InfoCards, { getInfoCards, ThirdPrivateCard } from './components/info-cards'
import InfoButton from './components/info-button'
import {
  useTorObserver,
  useInfoCardsDismissed,
  useShouldShowVpnPromo
} from './hooks'

// `margin: auto` centers the column when the window is tall enough and lets it
// top-align (and scroll) on short windows instead of being clipped. The gap
// below the hero lives on the info cards, so it can collapse with them.
const Column = styled.main<{ isTor: boolean }>`
  margin: auto;
  padding: ${p => p.isTor ? leo.spacing['8Xl'] : leo.spacing['3Xl']}
    ${leo.spacing['2Xl']} ${leo.spacing['3Xl']};
  display: flex;
  flex-direction: column;
  align-items: center;
  position: relative;
  max-width: 100%;
  box-sizing: border-box;
`

const Hero = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: ${leo.spacing['5Xl']};
  width: 640px;
  max-width: 100%;
`

const Heading = styled.h1`
  display: flex;
  align-items: center;
  gap: ${leo.spacing.xl};
  margin: 0;
  font: ${leo.font.heading.display1};
  letter-spacing: ${leo.typography.heading.display1.letterSpacing};
  color: ${leo.color.white};
  --leo-icon-size: 64px;
`

const BadgeBox = styled.div`
  position: absolute;
  top: ${leo.spacing.xl};
  left: ${leo.spacing.xl};
  z-index: 1;
`

function Container () {
  const isWindowTor = loadTimeData.getBoolean('isWindowTor')

  const { isConnected, progress, message, connectionStatus } = useTorObserver()
  const isTorDisabled = loadTimeData.getBoolean('isTorDisabled')
  const { isDismissed, setDismissed } = useInfoCardsDismissed()
  // The page opens in its saved state without motion; the cards and the info
  // button only animate once the user toggles them.
  const [hasToggledCards, setHasToggledCards] = React.useState(false)
  const { shouldShowVpnPromo } = useShouldShowVpnPromo(!isWindowTor)

  // While loading, the third slot is a skeleton rendered by InfoCards.
  const isVpnPromoLoading = !isWindowTor && shouldShowVpnPromo === null
  let thirdPrivateCard: ThirdPrivateCard = isTorDisabled ? 'none' : 'tor'
  if (isVpnPromoLoading) {
    thirdPrivateCard = 'none'
  } else if (shouldShowVpnPromo) {
    thirdPrivateCard = 'vpn'
  }

  const cards = React.useMemo(() => {
    if (isWindowTor) return getInfoCards({ isTor: true })
    return getInfoCards({
      isTor: false,
      thirdCard: thirdPrivateCard,
      onOpenVpnCheckout: () =>
        getPageHandlerInstance().pageHandler.openVpnCheckout(),
      onOpenTorWindow: () =>
        getPageHandlerInstance().pageHandler.openTorWindow()
    })
  }, [isWindowTor, thirdPrivateCard])

  const handleSearchSubmit = (value: string, openNewTab: boolean) => {
    getPageHandlerInstance().pageHandler.goToBraveSearch(value, openNewTab)
  }

  return (
    <BackgroundView isTor={isWindowTor}>
      {isWindowTor && (
        <BadgeBox>
          <BadgeTor
            isConnected={isConnected}
            progress={progress ?? ''}
            message={message ?? ''}
            connectionStatus={connectionStatus}
          />
        </BadgeBox>
      )}
      <Column isTor={isWindowTor}>
        <Hero>
          <Heading>
            <Icon name='social-brave-release-favicon-fullheight-color' />
            {getLocale(isWindowTor ? 'headingTor' : 'headingPrivate')}
          </Heading>
          <SearchBox onSubmit={handleSearchSubmit} />
        </Hero>
        {!isDismissed && (
          <InfoCards
            cards={cards}
            showSkeleton={isVpnPromoLoading}
            animateIn={hasToggledCards}
            onClose={() => {
              setHasToggledCards(true)
              setDismissed(true)
            }}
          />
        )}
      </Column>
      {isDismissed && (
        <InfoButton
          animateIn={hasToggledCards}
          onClick={() => {
            setHasToggledCards(true)
            setDismissed(false)
          }}
        />
      )}
    </BackgroundView>
  )
}

export default Container
