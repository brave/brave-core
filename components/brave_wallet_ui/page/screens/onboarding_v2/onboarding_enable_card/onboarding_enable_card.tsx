// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/nala/react/icon'

// Utils
import { getLocale, formatLocale } from '$web-common/locale'

// Components
import {
  OnboardingContentLayout, //
} from '../components/onboarding_content_layout/onboarding_content_layout'

// Assets
import MastercardMark from './mastercard.svg'

// Styled Components
import { Column, Row, Text } from '$wallet/components/shared/style'
import { Button } from '../onboarding.style'
import {
  Card,
  MastercardLogo,
  CardDetails,
  Tile,
  BenefitIconBubble,
  Description,
  NotNowButton,
  BraveCardTextBubble,
  PrintInCharacter,
  PrintInCharacters,
} from './onboarding_enable_card.style'

// Gaps ease from slow, through quick in the middle, back to slow.
const PRINT_IN_GAP_MIN_MS = 26
const PRINT_IN_GAP_MAX_MS = 72
const PRINT_IN_START_MS = 200

const prefersReducedMotion = () => {
  return window.matchMedia('(prefers-reduced-motion: reduce)').matches
}

// Group each space with the character that follows it. A space on its
// own would be a step where nothing new appears between words.
const printInUnits = (text: string) => {
  const units: string[] = []
  let pending = ''
  for (const char of text) {
    if (char === ' ') {
      pending += char
      continue
    }
    units.push(pending + char)
    pending = ''
  }
  if (pending) {
    units.push(pending)
  }
  return units
}

const printInDelays = (count: number) => {
  const delays: number[] = []
  let elapsed = 0
  for (let i = 0; i < count; i++) {
    delays.push(elapsed)
    const progress = count <= 1 ? 0.5 : i / (count - 1)
    const slowness = 0.5 + 0.5 * Math.cos(2 * Math.PI * progress)
    elapsed +=
      PRINT_IN_GAP_MIN_MS
      + (PRINT_IN_GAP_MAX_MS - PRINT_IN_GAP_MIN_MS) * slowness
  }
  return delays
}

const BraveCardPrintIn = ({ text }: { text: string }) => {
  const units = React.useMemo(() => printInUnits(text), [text])
  const [shown, setShown] = React.useState(() =>
    prefersReducedMotion() ? units.length : 0,
  )

  React.useEffect(() => {
    if (prefersReducedMotion()) {
      setShown(units.length)
      return
    }

    setShown(0)
    const delays = printInDelays(units.length)
    const timers = delays.map((delay, index) =>
      window.setTimeout(() => setShown(index + 1), PRINT_IN_START_MS + delay),
    )
    return () => {
      timers.forEach((timer) => window.clearTimeout(timer))
    }
  }, [units])

  return (
    <BraveCardTextBubble
      variant='heading.display3'
      aria-label={text}
    >
      <PrintInCharacters aria-hidden={true}>
        {units.slice(0, shown).map((unit, index) => (
          <PrintInCharacter key={`${unit}-${index}`}>{unit}</PrintInCharacter>
        ))}
      </PrintInCharacters>
    </BraveCardTextBubble>
  )
}

const PageTitle: React.FC<{}> = () => {
  const text = formatLocale(S.BRAVE_WALLET_ONBOARDING_ENABLE_CARD_TITLE, {
    $1: (content) => <BraveCardPrintIn text={content} />,
  })
  return (
    <Text
      variant='heading.display3'
      textColor='primary'
    >
      {text}
    </Text>
  )
}

const REWARDS_BENEFITS: { icon: string; title: string }[] = [
  {
    icon: 'shopping-bag',
    title: S.BRAVE_WALLET_ONBOARDING_ENABLE_CARD_CASHBACK_BENEFIT,
  },
  {
    icon: 'globe',
    title: S.BRAVE_WALLET_ONBOARDING_ENABLE_CARD_ADS_BENEFIT,
  },
  {
    icon: 'graph',
    title: S.BRAVE_WALLET_ONBOARDING_ENABLE_CARD_APY_BENEFIT,
  },
  {
    icon: 'check-circle-filled',
    title: S.BRAVE_WALLET_ONBOARDING_ENABLE_CARD_NO_FEES_BENEFIT,
  },
]

export const OnboardingEnableCard = () => {
  // Methods
  const onClickAddCard = () => {
    // Logic to add card will be handled in a future PR.
    console.log('add card')
  }

  const onClickNotNow = () => {
    // Logic for "not now" will be handled in a future PR.
    console.log('not now')
  }

  // Render
  return (
    <OnboardingContentLayout>
      <Column width='100%'>
        <Column gap='4px'>
          <Row>
            <Text
              variant='heading.display3'
              textColor='primary'
            >
              <PageTitle />
            </Text>
          </Row>
          <Description
            variant='large.regular'
            textColor='secondary'
          >
            {getLocale(S.BRAVE_WALLET_ONBOARDING_ENABLE_CARD_DESCRIPTION)}
          </Description>
        </Column>
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
            <CardDetails
              variant='small.regular'
              textColor='white'
            >
              {getLocale(S.BRAVE_WALLET_ONBOARDING_ENABLE_CARD_READY)}
            </CardDetails>
            <MastercardLogo
              src={MastercardMark}
              alt=''
            />
          </Row>
        </Card>
        <Column
          gap='12px'
          width='100%'
          alignItems='flex-start'
          margin='0px 0px 24px 0px'
        >
          {REWARDS_BENEFITS.map((benefit) => (
            <Row
              key={benefit.title}
              width='unset'
              gap='12px'
            >
              <BenefitIconBubble>
                <Icon name={benefit.icon} />
              </BenefitIconBubble>
              <Text
                variant='default.semibold'
                textColor='secondary'
              >
                {getLocale(benefit.title)}
              </Text>
            </Row>
          ))}
        </Column>
        <Column gap='6px'>
          <Button
            onClick={onClickAddCard}
            size='large'
          >
            {getLocale(S.BRAVE_WALLET_ONBOARDING_ENABLE_CARD_ADD_BUTTON)}
          </Button>
          <NotNowButton
            size='small'
            kind='plain-faint'
            onClick={onClickNotNow}
          >
            {getLocale(S.BRAVE_WALLET_ONBOARDING_ENABLE_CARD_NOT_NOW_BUTTON)}
          </NotNowButton>
        </Column>
      </Column>
    </OnboardingContentLayout>
  )
}
