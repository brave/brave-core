// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useLocation, useHistory } from 'react-router-dom'
import NavigationMenu from '@brave/leo/react/navigationMenu'
import NavigationItem from '@brave/leo/react/navigationItem'

// Utils
import { getLocale } from '../../../../common/locale'

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
  PanelOptionsWrapper,
  LeoNavigation,
  WalletLogo
} from './wallet-nav.style'
import { Row, VerticalDivider } from '../../shared/style'

export interface Props {
  isAndroid: boolean
}

export const WalletNav = (props: Props) => {
  const { isAndroid } = props

  // routing
  const history = useHistory()
  const { pathname: walletLocation } = useLocation()

  // computed
  const panelOrAndroidNavOptions = isAndroid ? NavOptions : PanelNavOptions

  return (
    <Wrapper>
      <PanelOptionsWrapper>
        <Section>
          {panelOrAndroidNavOptions.map((option) => (
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
            {NavOptions.map((option) => (
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
            {BuySendSwapDepositOptions.map((option) => (
              <NavigationItem
                key={option.id}
                icon={option.icon}
                isCurrent={walletLocation.includes(option.route)}
                onClick={() => history.push(option.route)}
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
