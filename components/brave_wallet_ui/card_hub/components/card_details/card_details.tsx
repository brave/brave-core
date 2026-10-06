// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

// Utils
import { getLocale, formatLocale } from '$web-common/locale'

// Components
import { Header } from '../header/header'
import { ActionButton } from './action_button/action_button'

// Styles
import { CardSlot, DetailsHeader, DetailsPane } from '../../card_hub.style'
import { Column, Row, Text } from '$wallet/components/shared/style'
import { BoxContainer, DetailsButton, IconBubble } from './card_details.style'

const earningsMonth = new Date().toLocaleDateString(undefined, {
  month: 'long',
  year: 'numeric',
})

// Placeholders until we have actual data
const monthlyBATEarned = '0.000'
const monthlyFiatEarned = '$0.00'
const monthlySpending = '$0.00'

interface Props {
  title: string
  visible: boolean
  slotRef: React.Ref<HTMLDivElement>
  onBack?: () => void
}

export const CardDetails = (props: Props) => {
  const { title, visible, slotRef, onBack } = props

  return (
    <DetailsPane $visible={visible}>
      <DetailsHeader>
        <Header
          title={title}
          onBack={onBack}
        />
      </DetailsHeader>
      <Column
        width='100%'
        padding='0 16px 16px 16px'
        gap='16px'
      >
        <CardSlot ref={slotRef} />
        <Row
          width='100%'
          padding='0 16px'
          justifyContent='space-between'
        >
          <ActionButton
            icon='plus-add'
            label={getLocale(S.BRAVE_WALLET_ADD_FUNDS)}
            onClick={() => {}}
          />
          <ActionButton
            icon='credit-card'
            label={getLocale(S.BRAVE_WALLET_NEW_CARD)}
            onClick={() => {}}
          />
          <ActionButton
            icon='file-text'
            label={getLocale(S.BRAVE_WALLET_ACTIVITY)}
            onClick={() => {}}
          />
          <ActionButton
            icon='more-horizontal'
            label={getLocale(S.BRAVE_WALLET_BUTTON_MORE)}
            onClick={() => {}}
          />
        </Row>
        <BoxContainer width='100%'>
          <DetailsButton>
            <Row
              gap='8px'
              width='unset'
            >
              <IconBubble>
                <Icon name='product-bat-color' />
              </IconBubble>
              <Column alignItems='flex-start'>
                <Text
                  variant='default.semibold'
                  textColor='primary'
                >
                  {formatLocale(S.BRAVE_WALLET_EARNINGS, {
                    $1: (content) => (
                      <Text
                        variant='default.regular'
                        textColor='tertiary'
                      >
                        {content}
                      </Text>
                    ),
                    $2: earningsMonth,
                  })}
                </Text>
                <Text
                  variant='small.regular'
                  textColor='tertiary'
                >
                  {getLocale(S.BRAVE_WALLET_INTEREST_PLUS_CASHBACK).replace(
                    '$1',
                    monthlyBATEarned,
                  )}
                </Text>
              </Column>
            </Row>
            <Text
              variant='default.semibold'
              textColor='primary'
            >
              {monthlyFiatEarned}
            </Text>
          </DetailsButton>
          <DetailsButton>
            <Row
              gap='8px'
              width='unset'
            >
              <IconBubble>
                <Icon name='product-brave-wallet' />
              </IconBubble>
              <Column alignItems='flex-start'>
                <Text
                  variant='default.semibold'
                  textColor='primary'
                >
                  {getLocale(S.BRAVE_WALLET_SPENDING)}
                </Text>
                <Text
                  variant='small.regular'
                  textColor='tertiary'
                >
                  {getLocale(S.BRAVE_WALLET_THIS_MONTH)}
                </Text>
              </Column>
            </Row>
            <Text
              variant='default.semibold'
              textColor='primary'
            >
              {monthlySpending}
            </Text>
          </DetailsButton>
          <DetailsButton>
            <Row
              gap='8px'
              width='unset'
            >
              <IconBubble>
                <Icon name='credit-card' />
              </IconBubble>
              <Column alignItems='flex-start'>
                <Text
                  variant='default.semibold'
                  textColor='primary'
                >
                  {getLocale(S.BRAVE_WALLET_GET_PHYSICAL_CARD)}
                </Text>
                <Text
                  variant='small.regular'
                  textColor='tertiary'
                >
                  {getLocale(S.BRAVE_WALLET_USE_IN_STORES_AND_ATMS)}
                </Text>
              </Column>
            </Row>
          </DetailsButton>
        </BoxContainer>
      </Column>
    </DetailsPane>
  )
}
