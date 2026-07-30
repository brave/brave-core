// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useLocation, useHistory } from 'react-router-dom'
import NavigationMenu from '@brave/leo/react/navigationMenu'
import NavigationItem from '@brave/leo/react/navigationItem'
import Icon from '@brave/leo/react/icon'

// Local Storage
import {
  useSyncedLocalStorage, //
} from '../../../common/hooks/use_local_storage'
import {
  LOCAL_STORAGE_KEYS, //
} from '../../../common/constants/local-storage-keys'

// Hooks
import { useRoute } from '../../../common/hooks/use_route'

// Selectors
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../common/selectors'

// Utils
import { getLocale } from '../../../../common/locale'

// Options
import {
  NavOptions,
  PanelNavOptions,
  BuySendSwapDepositOptions,
  BuySendSwapDepositIOSOptions,
} from '../../../options/nav-options'

// Components
import { WalletNavButton } from './wallet-nav-button/wallet-nav-button'

// Styled Components
import {
  Wrapper,
  Section,
  PageOptionsWrapper,
  PanelOptionsWrapper,
  LeoNavigation,
  WalletLogo,
  SidePanelWrapper,
  CloseButton,
  Header,
  BackgroundOverlay,
} from './wallet-nav.style'
import { Row, Text, VerticalDivider } from '../../shared/style'

export const WalletNav = () => {
  // UI Selectors (safe)
  const isPanel = useSafeUISelector(UISelectors.isPanel)
  const isSidePanel = useSafeUISelector(UISelectors.isSidePanel)
  const isIOS = useSafeUISelector(UISelectors.isIOS)

  // Local Storage
  const [isNavOpen, setIsNavOpen] = useSyncedLocalStorage(
    LOCAL_STORAGE_KEYS.IS_NAVIGATION_OPEN,
    false,
  )

  // routing
  const history = useHistory()
  const { pathname: walletLocation } = useLocation()
  const { openOrPushRoute } = useRoute()

  // computed
  const navigationOptions = isPanel ? PanelNavOptions : NavOptions

  const walletActionOptions = isIOS
    ? BuySendSwapDepositIOSOptions
    : BuySendSwapDepositOptions

  if (isSidePanel) {
    return (
      <>
        <SidePanelWrapper isOpen={isNavOpen}>
          <LeoNavigation>
            <Header
              justifyContent='flex-start'
              padding='16px'
              gap='12px'
              slot='header'
            >
              <CloseButton
                kind='plain-faint'
                fab
                onClick={() => setIsNavOpen(false)}
              >
                <Icon name='close' />
              </CloseButton>
              <Text
                textColor='primary'
                variant='heading.h4'
              >
                {getLocale('braveWalletTitle')}
              </Text>
            </Header>
            <NavigationMenu>
              {navigationOptions.map((option) => (
                <NavigationItem
                  key={option.id}
                  icon={option.icon}
                  isCurrent={walletLocation.includes(option.route)}
                  onClick={() => {
                    history.push(option.route)
                    setIsNavOpen(false)
                  }}
                >
                  {getLocale(option.name)}
                </NavigationItem>
              ))}
              <Row>
                <VerticalDivider />
              </Row>
              {walletActionOptions.map((option) => (
                <NavigationItem
                  key={option.id}
                  icon={option.icon}
                  isCurrent={walletLocation.includes(option.route)}
                  onClick={() => {
                    openOrPushRoute(option.route)
                    setIsNavOpen(false)
                  }}
                >
                  {getLocale(option.name)}
                </NavigationItem>
              ))}
            </NavigationMenu>
          </LeoNavigation>
        </SidePanelWrapper>
        <BackgroundOverlay
          isOpen={isNavOpen}
          onClick={() => setIsNavOpen(false)}
        />
      </>
    )
  }

  return (
    <Wrapper>
      <PanelOptionsWrapper>
        <Section>
          {navigationOptions.map((option) => (
            <WalletNavButton
              option={option}
              key={option.id}
            />
          ))}
        </Section>
      </PanelOptionsWrapper>

      <PageOptionsWrapper>
        <LeoNavigation>
          <Row
            justifyContent='flex-start'
            padding='32px 0px 16px 24px'
            slot='header'
          >
            <WalletLogo />
          </Row>
          <NavigationMenu>
            {navigationOptions.map((option) => (
              <NavigationItem
                key={option.id}
                icon={option.icon}
                isCurrent={walletLocation.includes(option.route)}
                onClick={() => history.push(option.route)}
              >
                {getLocale(option.name)}
              </NavigationItem>
            ))}
            <Row>
              <VerticalDivider />
            </Row>
            {walletActionOptions.map((option) => (
              <NavigationItem
                key={option.id}
                icon={option.icon}
                isCurrent={walletLocation.includes(option.route)}
                onClick={() => openOrPushRoute(option.route)}
              >
                {getLocale(option.name)}
              </NavigationItem>
            ))}
          </NavigationMenu>
        </LeoNavigation>
      </PageOptionsWrapper>
    </Wrapper>
  )
}

export default WalletNav
