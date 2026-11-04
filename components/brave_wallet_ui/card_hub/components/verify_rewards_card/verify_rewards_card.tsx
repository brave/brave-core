// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'

// Utils
import { getLocale } from '$web-common/locale'

// Components
import { Header } from '../header/header'

// Styles
import {
  CardSlot,
  DetailsHeader,
  DetailsPane,
} from '$wallet/card_hub/card_hub.style'
import { Column, Row, Text } from '$wallet/components/shared/style'
import {
  IconContainer,
  ContinueButtonWrapper,
} from './verify_rewards_card.style'

interface Props {
  visible: boolean
  slotRef: React.Ref<HTMLDivElement>
  onBack?: () => void
}

export const VerifyRewardsCard = (props: Props) => {
  const { visible, slotRef, onBack } = props

  const onContinue = () => {
    // Will navigate to the rewards card kyc page once the page is implemented
    console.log('onContinue')
  }

  return (
    <DetailsPane $visible={visible}>
      <DetailsHeader>
        <Header
          title={getLocale(S.BRAVE_WALLET_VERIFY_CARD)}
          onBack={onBack}
        />
      </DetailsHeader>
      <Column
        width='100%'
        padding='0 16px 16px 16px'
        gap='16px'
      >
        <CardSlot ref={slotRef} />
        <Text
          variant='default.regular'
          textColor='secondary'
          textAlign='left'
        >
          {getLocale(S.BRAVE_WALLET_VERIFY_CARD_DESCRIPTION)}
        </Text>
        <Row
          justifyContent='flex-start'
          alignItems='flex-start'
          gap='8px'
        >
          <IconContainer>
            <Icon name='shield-done' />
          </IconContainer>
          <Column alignItems='flex-start'>
            <Text
              variant='default.semibold'
              textColor='primary'
              textAlign='left'
            >
              {getLocale(S.BRAVE_WALLET_VERIFY_CARD_REQUIRED_TITLE)}
            </Text>
            <Text
              variant='small.regular'
              textColor='tertiary'
              textAlign='left'
            >
              {getLocale(S.BRAVE_WALLET_VERIFY_CARD_REQUIRED_DESCRIPTION)}
            </Text>
          </Column>
        </Row>
        <Row
          justifyContent='flex-start'
          alignItems='flex-start'
          gap='8px'
        >
          <IconContainer>
            <Icon name='camera' />
          </IconContainer>
          <Column alignItems='flex-start'>
            <Text
              variant='default.semibold'
              textColor='primary'
              textAlign='left'
            >
              {getLocale(S.BRAVE_WALLET_VERIFY_CARD_PHOTO_ID_TITLE)}
            </Text>
            <Text
              variant='small.regular'
              textColor='tertiary'
              textAlign='left'
            >
              {getLocale(S.BRAVE_WALLET_VERIFY_CARD_PHOTO_ID_DESCRIPTION)}
            </Text>
          </Column>
        </Row>
        <Row
          justifyContent='flex-start'
          alignItems='flex-start'
          gap='8px'
        >
          <IconContainer>
            <Icon name='launch' />
          </IconContainer>
          <Column alignItems='flex-start'>
            <Text
              variant='default.semibold'
              textColor='primary'
              textAlign='left'
            >
              {getLocale(S.BRAVE_WALLET_VERIFY_CARD_NEW_TAB_TITLE)}
            </Text>
            <Text
              variant='small.regular'
              textColor='tertiary'
              textAlign='left'
            >
              {getLocale(S.BRAVE_WALLET_VERIFY_CARD_NEW_TAB_DESCRIPTION)}
            </Text>
          </Column>
        </Row>
      </Column>
      <ContinueButtonWrapper padding='0 16px 16px 16px'>
        <Button onClick={onContinue}>
          {getLocale(S.BRAVE_WALLET_BUTTON_CONTINUE)}
        </Button>
      </ContinueButtonWrapper>
    </DetailsPane>
  )
}
