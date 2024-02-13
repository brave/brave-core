// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled, { css } from 'styled-components'
import Flex from '$web-common/Flex'
import PublisherCard from '../../../../../brave_news/browser/resources/shared/PublisherCard'
import { ArrowRight } from '../../../../../brave_news/browser/resources/shared/Icons'

const CARD_SIZE = 208
const CARD_SIZE_PX = `${CARD_SIZE}px`
const CARD_GAP = '16px'

const ScrollButton = styled.button<{ hidden: boolean }>`
  all: unset;
  position: absolute;
  width: 32px;
  height: 32px;
  top: 32px;
  background: white;
  border-radius: 32px;
  box-shadow: 0px 1px 4px rgba(63, 76, 99, 0.35);
  display: flex;
  align-items: center;
  justify-content: center;
  color: var(--text2);
  cursor: pointer;

  :hover {
    box-shadow: 0px 1px 4px rgba(63, 76, 99, 0.5);
    color: var(--interactive4);
  }

  ${p => p.hidden && css`opacity: 0;`}

  transition: opacity 0.2s ease-in-out, color 0.2s ease-in-out;
`

const ScrollButtonLeft = styled(ScrollButton)`
  left: -16px;
  transform: rotate(180deg);
`

const ScrollButtonRight = styled(ScrollButton)`
  right: -16px;
`

const Container = styled(Flex)`
  padding: 16px 0;
  max-width: calc(${CARD_SIZE_PX} * 3 + ${CARD_GAP} * 2);
  container-name: carousel;
  container-type: inline-size;
  &:not(:hover, :has(:focus-visible)) ${ScrollButton} {
    opacity: 0;
  }
`

const Header = styled.div`
  width: 100%;
  font-weight: 600;
  font-size: 16px;
  margin: 8px 0;
`

const Subtitle = styled.span`
  font-size: 12px;
`

const CarouselContainer = styled.div`
  position: relative;
`

const ItemsContainer = styled(Flex)`
  margin: 8px 0;
  overflow-x: auto;
  overflow-y: hidden;
  scroll-snap-type: x mandatory;

  &::-webkit-scrollbar {
   display: none;
   width: 0;
  }
`

const PublisherCardContainer = styled.div`
  min-width: calc((100cqi - ${CARD_GAP} * 2) / 3);
  max-width: ${CARD_SIZE_PX};
  scroll-snap-align: start;
`

interface Props {
  title: string | JSX.Element
  subtitle?: React.ReactNode
  publisherIds: string[]
}

export default function Carousel(props: Props) {
  const scrollContainerRef = React.useRef<HTMLDivElement>()
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
      <Flex direction='row' gap={8} align='center'>
        <Header>{props.title}</Header>
      </Flex>
      {props.subtitle && <Subtitle>
        {props.subtitle}
      </Subtitle>}
      <CarouselContainer>
        <ItemsContainer direction='row' gap={CARD_GAP} ref={scrollContainerRef as any} onScroll={updateAvailableDirections}>
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
