/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { style } from './source_carousel.style'

interface Props {
  children: React.ReactNode
}

export function SourceCarousel(props: Props) {
  const itemsRef = React.useRef<HTMLDivElement>(null)

  function updateScrollState() {
    const elem = itemsRef.current
    if (!elem) {
      return
    }
    const scrollLeft = Math.abs(elem.scrollLeft)
    elem.classList.toggle('can-scroll-back', scrollLeft > 0)
    elem.classList.toggle(
      'can-scroll-forward',
      scrollLeft + elem.clientWidth < elem.scrollWidth,
    )
  }

  function onClickLeft() {
    const elem = itemsRef.current
    if (elem) {
      scrollItem(elem, 'left')
    }
  }

  function onClickRight() {
    const elem = itemsRef.current
    if (elem) {
      scrollItem(elem, 'right')
    }
  }

  return (
    <div
      data-css-scope={style.scope}
      onMouseEnter={updateScrollState}
    >
      <div
        ref={itemsRef}
        className='items'
        onScroll={updateScrollState}
      >
        {props.children}
      </div>
      <button
        className='left'
        onClick={onClickLeft}
      >
        <Icon name='carat-left' />
      </button>
      <button
        className='right'
        onClick={onClickRight}
      >
        <Icon name='carat-right' />
      </button>
    </div>
  )
}

function scrollItem(parent: Element, dir: 'left' | 'right') {
  const item = parent.firstElementChild as HTMLElement | null
  if (!item) {
    return
  }
  const width = item.offsetWidth
  if (width <= 0) {
    return
  }
  parent.scrollBy({
    behavior: 'smooth',
    left: width * (dir === 'left' ? -1 : 1),
  })
}
