// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { createPortal } from 'react-dom'
import Button from '@brave/nala/react/button'
import Icon from '@brave/nala/react/icon'
import { getLocale } from '$web-common/locale'
import { RecapStats } from '../stats'
import '../strings'

export interface Nav {
  next: () => void
  back: () => void
  // Goes back from the start third and advances elsewhere. Ignored after a
  // hold.
  tap: (e: React.MouseEvent<HTMLElement>) => void
  setHelpOpen: (open: boolean) => void
}

export interface SlideProps {
  stats: RecapStats
  nav: Nav
  // The share image, rendered when the recap opens so it's ready on arrival.
  card?: ShareCard
}

export interface ShareCard {
  blob: Blob
  url: string
}

// Each slide renders inside a frame. Only the current frame owns the shared
// bottom bar, so the buttons stay put while slide content moves.
export interface Frame {
  exiting: boolean
  bottomBar: HTMLElement | null
}

export const FrameContext = React.createContext<Frame>({
  exiting: false,
  bottomBar: null,
})

export function SlideLayout(props: {
  scope?: string
  children: React.ReactNode
  start: React.ReactNode
  primary: React.ReactNode
  onContentClick?: (e: React.MouseEvent<HTMLElement>) => void
}) {
  const frame = React.useContext(FrameContext)
  return (
    <>
      <div
        className='slide'
        data-css-scope={props.scope}
        onClick={props.onContentClick}
      >
        {props.children}
      </div>
      {!frame.exiting
        && frame.bottomBar
        && createPortal(
          <>
            {props.start}
            {props.primary}
          </>,
          frame.bottomBar,
        )}
    </>
  )
}

export function BackButton({ nav }: { nav: Nav }) {
  return (
    <Button
      fab
      className='icon-button'
      kind='filled'
      size='medium'
      title={getLocale(S.BRAVE_RECAP_BACK)}
      aria-label={getLocale(S.BRAVE_RECAP_BACK)}
      onClick={nav.back}
      data-testid='recap-back'
    >
      <Icon name='carat-left' />
    </Button>
  )
}

export function NextButton({ nav }: { nav: Nav }) {
  return (
    <Button
      kind='filled'
      size='medium'
      onClick={nav.next}
    >
      {getLocale(S.BRAVE_RECAP_NEXT)}
      <Icon
        slot='icon-after'
        name='carat-right'
      />
    </Button>
  )
}
