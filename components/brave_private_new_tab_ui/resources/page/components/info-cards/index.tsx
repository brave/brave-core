// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled, { css, keyframes } from 'styled-components'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import * as leo from '@brave/leo/tokens/css/variables'
import { getLocale, formatLocale } from '$web-common/locale'

export type ThirdPrivateCard = 'vpn' | 'tor' | 'none'

export interface InfoCard {
  id: string
  icon: string
  title: string
  description: React.ReactNode
}

type GetInfoCardsOptions =
  | { isTor: true }
  | {
      isTor: false
      thirdCard: ThirdPrivateCard
      onOpenVpnCheckout: () => void
      onOpenTorWindow: () => void
    }

const HOVER_AREA_OUTSET = '50px'
// Room for the close button just outside the cards' top-right corner. Kept
// inside the box (and mirrored on the left) so the cards stay centered and
// the button is clipped by the close animation instead of popping out.
const FRAME_TOP = '30px'
const FRAME_X = '28px'

const LinkButton = styled.button`
  all: unset;
  color: var(--window-accent);
  text-decoration: underline;
  cursor: pointer;

  &:focus-visible {
    box-shadow: ${leo.effect.focusState};
    border-radius: ${leo.radius.xs};
  }
`

function withLink (key: string, onClick: () => void) {
  return formatLocale(key, {
    $1: (content) => <LinkButton onClick={onClick}>{content}</LinkButton>
  })
}

export function getInfoCards (options: GetInfoCardsOptions): InfoCard[] {
  const noTrace: InfoCard = {
    id: 'no-trace',
    icon: 'product-private-window',
    title: getLocale('cardNoTraceTitle'),
    description: getLocale('cardNoTraceDesc')
  }

  if (options.isTor) {
    return [
      noTrace,
      {
        id: 'about-tor',
        icon: 'product-tor',
        title: getLocale('cardAboutTorTitle'),
        description: getLocale('cardAboutTorDesc')
      },
      {
        id: 'tor-safety',
        icon: 'hearts',
        title: getLocale('cardTorSafetyTitle'),
        description: getLocale('cardTorSafetyDesc')
      }
    ]
  }

  const cards: InfoCard[] = [
    noTrace,
    {
      id: 'visibility',
      icon: 'antenna',
      title: getLocale('cardVisibilityTitle'),
      description: getLocale('cardVisibilityDesc')
    }
  ]

  if (options.thirdCard === 'vpn') {
    cards.push({
      id: 'vpn',
      icon: 'product-vpn',
      title: getLocale('cardVpnTitle'),
      description: withLink('cardVpnDesc', options.onOpenVpnCheckout)
    })
  } else if (options.thirdCard === 'tor') {
    cards.push({
      id: 'tor-promo',
      icon: 'product-tor',
      title: getLocale('cardTorPromoTitle'),
      description: withLink('cardTorPromoDesc', options.onOpenTorWindow)
    })
  }

  return cards
}

// The page column is vertically centered, so fading the cards out and then
// unmounting them makes the heading jump. The frame collapses with the cards.
const Collapse = styled.section`
  position: relative;
  box-sizing: border-box;
  width: max-content;
  max-width: 100%;
  min-width: 0;
  /* Lets the close animation shrink the box below its content height. */
  min-height: 0;
  margin-top: calc(${leo.spacing['7Xl']} - ${FRAME_TOP});
  padding: ${FRAME_TOP} ${FRAME_X} 0;

  &::before {
    content: '';
    position: absolute;
    z-index: 0;
    top: calc(${FRAME_TOP} - ${HOVER_AREA_OUTSET});
    right: calc(${FRAME_X} - ${HOVER_AREA_OUTSET});
    bottom: -${HOVER_AREA_OUTSET};
    left: calc(${FRAME_X} - ${HOVER_AREA_OUTSET});
  }
`

const CloseButtonBox = styled.div`
  position: absolute;
  z-index: 1;
  top: 0;
  right: 0;
  opacity: 0;
  transition: opacity ${leo.duration.s} ${leo.easing.inOut};

  ${Collapse}:hover &,
  ${Collapse}:focus-within & {
    opacity: 1;
  }

  leo-button {
    --leo-button-color: var(--window-surface);
  }

  leo-icon {
    --leo-icon-color: var(--window-accent);
    --leo-icon-size: var(--leo-icon-xs);
  }
`

const CardsRow = styled.ul`
  display: flex;
  flex-wrap: wrap;
  justify-content: center;
  align-items: stretch;
  gap: ${leo.spacing.xl};
  position: relative;
  z-index: 1;
  min-width: 0;
  margin: 0;
  padding: 0;
  list-style: none;
`

const fadeIn = keyframes`
  from {
    opacity: 0;
  }
`

const pulse = keyframes`
  to {
    opacity: 0.5;
  }
`

const Card = styled.li<{ isAppearing?: boolean }>`
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: ${leo.spacing.xl};
  width: 304px;
  box-sizing: border-box;
  padding: ${leo.spacing['2Xl']};
  border: 1px solid var(--window-surface);
  border-radius: ${leo.radius.xl};
  text-align: center;

  ${p => p.isAppearing && css`
    animation: ${fadeIn} ${leo.duration.l} ${leo.easing.inOut};
  `}

  @media (prefers-reduced-motion: reduce) {
    animation: none;
  }
`

const SkeletonCard = styled(Card)`
  animation: ${pulse} ${leo.duration.xl} ${leo.easing.inOut} infinite alternate;
`

const SkeletonShape = styled.div<{ width: string, height: string }>`
  width: ${p => p.width};
  max-width: 100%;
  height: ${p => p.height};
  border-radius: ${leo.radius.s};
  background: var(--window-surface);
`

const SkeletonText = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: ${leo.spacing.m};
  width: 100%;
`

function InfoCardSkeleton () {
  return (
    <SkeletonCard aria-hidden data-testid='info-card-skeleton'>
      <SkeletonShape
        width='56px'
        height='56px'
        style={{ borderRadius: leo.radius.full }}
      />
      <SkeletonText>
        <SkeletonShape
          width='60%'
          height={leo.typography.heading.h4.fontSize}
        />
        <SkeletonShape
          width='100%'
          height={leo.typography.small.regular.fontSize}
        />
        <SkeletonShape
          width='80%'
          height={leo.typography.small.regular.fontSize}
        />
      </SkeletonText>
    </SkeletonCard>
  )
}

const CardIcon = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  width: 56px;
  height: 56px;
  border-radius: ${leo.radius.full};
  background: var(--window-surface);
  --leo-icon-size: 36px;
  --leo-icon-color: var(--window-accent);
`

const CardText = styled.div`
  display: flex;
  flex-direction: column;
  gap: ${leo.spacing.s};

  h2 {
    margin: 0;
    font: ${leo.font.heading.h4};
    letter-spacing: ${leo.typography.heading.h4.letterSpacing};
    color: var(--window-text-primary);
  }

  p {
    margin: 0;
    font: ${leo.font.small.regular};
    letter-spacing: ${leo.typography.small.regular.letterSpacing};
    color: var(--window-text-secondary);
  }
`

interface Props {
  cards: InfoCard[]
  onClose: () => void
  // Adds a placeholder card after `cards` while one is still being resolved.
  showSkeleton?: boolean
  // Set when the info button is bringing the cards back. The first paint of a
  // new tab stays put; only that reopen runs the close animation in reverse.
  animateIn?: boolean
}

function motionTiming (el: HTMLElement) {
  const computed = getComputedStyle(el)
  return {
    duration:
      Number.parseFloat(computed.getPropertyValue('--leo-duration-l')) || 300,
    easing:
      computed.getPropertyValue('--leo-easing-in-out').trim() || 'ease-in-out',
    reduced: window.matchMedia('(prefers-reduced-motion: reduce)').matches
  }
}

// The animated values, so a close started mid-open continues from what's on
// screen instead of jumping to the full size.
function snapshot (el: HTMLElement) {
  const computed = getComputedStyle(el)
  return {
    height: `${el.getBoundingClientRect().height}px`,
    marginTop: computed.marginTop,
    paddingTop: computed.paddingTop,
    paddingRight: computed.paddingRight,
    paddingLeft: computed.paddingLeft,
    opacity: computed.opacity
  }
}

const collapsed = {
  height: '0px',
  marginTop: '0px',
  paddingTop: '0px',
  paddingRight: '0px',
  paddingLeft: '0px',
  opacity: 0
}

function InfoCards (props: Props) {
  const collapseRef = React.useRef<HTMLElement>(null)
  const animationRef = React.useRef<Animation | null>(null)
  const closingRef = React.useRef(false)

  // The cards on screen next to the skeleton. Whatever replaces it fades in,
  // while these stay put.
  const idsBesideSkeletonRef = React.useRef<Set<string> | null>(null)
  if (props.showSkeleton) {
    idsBesideSkeletonRef.current = new Set(props.cards.map(card => card.id))
  }
  const isAppearing = (id: string) =>
    !props.showSkeleton && !!idsBesideSkeletonRef.current &&
    !idsBesideSkeletonRef.current.has(id)

  React.useLayoutEffect(() => {
    if (!props.animateIn) return
    const el = collapseRef.current
    if (!el || motionTiming(el).reduced) return

    const { duration, easing } = motionTiming(el)
    const expanded = snapshot(el)
    el.style.overflow = 'hidden'
    const animation = el.animate([collapsed, expanded], {
      duration,
      easing,
      fill: 'both'
    })
    animationRef.current = animation
    animation.onfinish = () => {
      if (animationRef.current !== animation) return
      el.style.overflow = ''
      animation.cancel()
      animationRef.current = null
    }
  }, [props.animateIn])

  const handleClose = () => {
    if (closingRef.current) return
    closingRef.current = true

    const el = collapseRef.current
    if (!el || motionTiming(el).reduced) {
      props.onClose()
      return
    }

    const expanded = snapshot(el)
    animationRef.current?.cancel()
    const { duration, easing } = motionTiming(el)

    el.style.pointerEvents = 'none'
    el.style.overflow = 'hidden'
    const animation = el.animate([expanded, collapsed], {
      duration,
      easing,
      fill: 'both'
    })
    animationRef.current = animation
    animation.onfinish = () => props.onClose()
  }

  return (
    <Collapse ref={collapseRef} data-testid='info-cards'>
      <CardsRow>
        {props.cards.map(card => (
          <Card
            key={card.id}
            isAppearing={isAppearing(card.id)}
            data-testid={`info-card-${card.id}`}
          >
            <CardIcon>
              <Icon name={card.icon} />
            </CardIcon>
            <CardText>
              <h2>{card.title}</h2>
              <p>{card.description}</p>
            </CardText>
          </Card>
        ))}
        {props.showSkeleton && <InfoCardSkeleton />}
      </CardsRow>
      <CloseButtonBox>
        <Button
          fab
          kind='filled'
          size='tiny'
          onClick={handleClose}
          title={getLocale('hideInfoLabel')}
          aria-label={getLocale('hideInfoLabel')}
          data-testid='info-cards-close-button'
        >
          <Icon name='close' />
        </Button>
      </CloseButtonBox>
    </Collapse>
  )
}

export default InfoCards
