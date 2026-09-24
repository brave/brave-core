// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled, { css } from 'styled-components'
import Flex from '$web-common/Flex'
import PublisherCard from '../shared/PublisherCard'
import { ArrowRight } from '../shared/Icons'
import { color, effect, font, radius, spacing } from '@brave/leo/tokens/css/variables'

const CARD_SIZE = 208
const CARD_SIZE_PX = `${CARD_SIZE}px`

const ScrollButton = styled.button<{ hidden: boolean }>`
  all: unset;
  position: absolute;
  width: ${spacing['3Xl']};
  height: ${spacing['3Xl']};
  top: ${spacing['3Xl']};
  background: ${color.container.background};
  border-radius: ${radius.full};
  box-shadow: ${effect.elevation['02']};
  display: flex;
  align-items: center;
  justify-content: center;
  color: ${color.icon.default};
  cursor: pointer;

  :hover {
    box-shadow: ${effect.elevation['03']};
    color: ${color.icon.interactive};
  }

  ${p => p.hidden && css`opacity: 0;`}

  transition: opacity 0.2s ease-in-out, color 0.2s ease-in-out;
`

const ScrollButtonLeft = styled(ScrollButton)`
  left: calc(-1 * ${spacing.xl});
  transform: rotate(180deg);
`

const ScrollButtonRight = styled(ScrollButton)`
  right: calc(-1 * ${spacing.xl});
`

const Container = styled(Flex)`
  padding: ${spacing.xl} 0 0 0;
  max-width: calc(${CARD_SIZE_PX} * 3 + ${spacing.xl} * 2);
  container-name: carousel;
  container-type: inline-size;
  &:not(:hover, :has(:focus-visible)) ${ScrollButton} {
    opacity: 0;
  }
`

const Header = styled.div`
  width: 100%;
  font: ${font.heading.h4};
  margin: ${spacing.m} 0;
`

const Subtitle = styled.span`
  font: ${font.small.regular};
`

const CarouselContainer = styled.div`
  position: relative;
`

const ItemsContainer = styled(Flex)`
  margin: ${spacing.m} 0;
  overflow-x: auto;
  overflow-y: hidden;
  scroll-snap-type: x mandatory;

  &::-webkit-scrollbar {
   display: none;
   width: 0;
  }
`

const PublisherCardContainer = styled.div`
  min-width: calc((100cqi - ${spacing.xl} * 2) / 3);
  max-width: ${CARD_SIZE_PX};
  scroll-snap-align: start;
`

interface Props {
  title: string | JSX.Element
  subtitle?: React.ReactNode
  publisherIds: string[]
}

export default function Carousel(props: Props) {
  const scrollContainerRef = React.useRef<HTMLDivElement>(null)
  const [availableDirections, setAvailableDirections] = React.useState<'none' | 'left' | 'right' | 'both'>('right')
  const updateAvailableDirections = React.useCallback(() => {
    if (!scrollContainerRef.current) return

    const end = scrollContainerRef.current.scrollWidth - scrollContainerRef.current.clientWidth
    const scrollPos = scrollContainerRef.current.scrollLeft
    if (end <= 0) {
      setAvailableDirections('none')
    } else if (end > scrollPos && scrollPos > 0) {
      setAvailableDirections('both')
    } else if (end > scrollPos) {
      setAvailableDirections('right')
    } else {
      setAvailableDirections('left')
    }
  }, [])

  const scroll = React.useCallback((dir: 'left' | 'right') => {
    if (!scrollContainerRef.current) return

    scrollContainerRef.current.scrollBy({
      behavior: 'smooth',
      left: CARD_SIZE * (dir === 'left' ? -1 : 1)
    })
  }, [])

  if (!props.publisherIds.length) {
    return null
  }

  return (
    <Container direction='column'>
      <Flex direction='row' gap={spacing.m} align='center'>
        <Header>{props.title}</Header>
      </Flex>
      {props.subtitle && <Subtitle>
        {props.subtitle}
      </Subtitle>}
      <CarouselContainer>
        <ItemsContainer direction='row' gap={spacing.xl} ref={scrollContainerRef as any} onScroll={updateAvailableDirections}>
          {props.publisherIds.map(p => <PublisherCardContainer key={p}>
            <PublisherCard publisherId={p} />
          </PublisherCardContainer>)}
        </ItemsContainer>
        <ScrollButtonLeft onClick={() => scroll('left')} hidden={availableDirections === 'right' || availableDirections === 'none'}>
          {ArrowRight}
        </ScrollButtonLeft>
        <ScrollButtonRight onClick={() => scroll('right')} hidden={availableDirections === 'left' || availableDirections === 'none'}>
          {ArrowRight}
        </ScrollButtonRight>
      </CarouselContainer>
    </Container>
  )
}
