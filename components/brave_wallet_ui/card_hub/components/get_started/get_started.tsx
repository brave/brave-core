// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/nala/react/icon'

// Assets
import {
  CreateETHIcon,
  CreateSOLIcon,
  CreateBTCIcon,
} from '$wallet/assets/create_account_icons/create_account_icons'
import MastercardMark from '$wallet/assets/svg-icons/mastercard.svg'
import VisaMark from '$wallet/assets/svg-icons/visa.svg'

// Utils
import { getLocale } from '$web-common/locale'

// Styled Components
import { Column, Row, Text } from '$wallet/components/shared/style'
import {
  BackingCard,
  Body,
  Button,
  Card,
  BatIcon,
  Floater,
  Glow,
  Hero,
  NetworkIcon,
  Screen,
  Tile,
  CardLogo,
} from './get_started.style'
import { WalletLogo } from '$wallet/card_hub/card_hub.style'

export const GetStarted = () => {
  // Methods
  const onClickGetStarted = () => {
    // Logic to navigate to the onboarding screen
    // will be implemented in the future.
    console.log('onClickGetStarted')
  }

  return (
    <Screen>
      <Glow
        aria-hidden
        $corner='topLeft'
      />
      <Glow
        aria-hidden
        $corner='bottomRight'
      />
      <Body
        justifyContent='flex-start'
        width='100%'
        height='100%'
        padding='24px 16px 16px'
      >
        <WalletLogo />
        <Hero>
          <BackingCard
            $x={48}
            $y={16}
            $rotate={9}
            $zIndex={1}
            $delay={0.9}
            $graphic='crypto'
          />
          <BackingCard
            $x={-46}
            $y={26}
            $rotate={-14}
            $zIndex={2}
            $delay={1.8}
          >
            <CardLogo
              src={VisaMark}
              alt=''
              $height={16}
              $invertOnDark
            />
            <Text
              variant='xSmall.semibold'
              textColor='secondary'
            >
              {getLocale(
                S.BRAVE_WALLET_CARD_HUB_GET_STARTED_SAMPLE_CARD_NUMBER,
              )}
            </Text>
          </BackingCard>
          <Card>
            <Row
              gap='8px'
              justifyContent='flex-start'
            >
              <Tile>
                <Icon name='brave-icon-release-color' />
              </Tile>
              <Tile>
                <Icon name='product-bat-color' />
              </Tile>
            </Row>
            <Row
              gap='8px'
              alignItems='flex-end'
              justifyContent='space-between'
            >
              <Text
                variant='small.semibold'
                textColor='white'
              >
                {getLocale(S.BRAVE_WALLET_BRAVE_REWARDS_TITLE)}
              </Text>
              <CardLogo
                src={MastercardMark}
                alt=''
              />
            </Row>
          </Card>
          <Floater
            $top='4px'
            $left='8px'
            $delay={0}
          >
            <BatIcon name='product-bat-color' />
          </Floater>
          <Floater
            $top='22px'
            $right='8px'
            $delay={0.4}
          >
            <NetworkIcon
              src={CreateETHIcon}
              $size={30}
            />
          </Floater>
          <Floater
            $bottom='56px'
            $right='10px'
            $delay={0.8}
            $zIndex={0}
          >
            <NetworkIcon
              src={CreateBTCIcon}
              $size={26}
            />
          </Floater>
          <Floater
            $bottom='50px'
            $left='4px'
            $delay={1.2}
          >
            <NetworkIcon
              src={CreateSOLIcon}
              $size={24}
            />
          </Floater>
        </Hero>
        <Column
          width='100%'
          padding='0 40px'
        >
          <Column
            margin='0 0 24px 0'
            gap='4px'
          >
            <Text
              variant='heading.h3'
              textColor='primary'
            >
              {getLocale(S.BRAVE_WALLET_CARD_HUB_GET_STARTED_TITLE)}
            </Text>
            <Text
              variant='small.regular'
              textColor='secondary'
            >
              {getLocale(S.BRAVE_WALLET_CARD_HUB_GET_STARTED_DESCRIPTION)}
            </Text>
          </Column>
          <Row margin='0 0 12px 0'>
            <Button onClick={onClickGetStarted}>
              {getLocale(S.BRAVE_WALLET_CARD_HUB_GET_STARTED_BUTTON)}
            </Button>
          </Row>
          <Text
            variant='xSmall.regular'
            textColor='tertiary'
          >
            {getLocale(S.BRAVE_WALLET_CARD_HUB_GET_STARTED_DURATION)}
          </Text>
        </Column>
      </Body>
    </Screen>
  )
}
