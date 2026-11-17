// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale, formatLocale } from '$web-common/locale'
import { RecapStats, hasEnoughData } from '../stats'
import { formatCount } from '../format'
import { ShareCardImages } from '../share_card'
import { DotMatrix } from './dot_matrix'
import { FrameContext, Nav, SlideProps } from './slide_layout'
import { EmptySlide, IntroSlide } from './intro_slide'
import { BlockedSlide, DataSlide, TimeSlide } from './stat_slide'
import { ShareSlide, useShareCard } from './share_slide'
import { useMediaQuery, usePageHidden } from './hooks'
import cardPattern from '../images/card_pattern.webp'
import { style } from './app.style'
import '../strings'

// Same-origin so they don't taint the share card canvas.
const CARD_IMAGES: ShareCardImages = {
  logo: '/brave-icon.svg',
  pattern: cardPattern,
}

export const SLIDES = ['intro', 'blocked', 'time', 'data', 'share'] as const
export type Slide = (typeof SLIDES)[number]
// These wait for the person: the intro until they start, the share slide
// because it's the end.
const UNTIMED_SLIDES: readonly Slide[] = ['intro', 'share']

// A press longer than this pauses the slide rather than advancing it.
const HOLD_MS = 300
// How much of the slide, from the start side, goes back when tapped.
const BACK_ZONE = 1 / 3

function Progress(props: {
  index: number
  autoAdvance: boolean
  paused: boolean
  onComplete: () => void
}) {
  return (
    <div
      className='progress'
      role='progressbar'
      aria-label={getLocale(S.BRAVE_RECAP_SLIDES_LABEL)}
      aria-valuemin={1}
      aria-valuemax={SLIDES.length}
      aria-valuenow={props.index + 1}
      aria-valuetext={formatLocale(S.BRAVE_RECAP_SLIDE_LABEL, {
        $1: formatCount(props.index + 1),
        $2: formatCount(SLIDES.length),
      })}
    >
      {SLIDES.map((slide, i) => {
        const state =
          i < props.index
            ? 'done'
            : i > props.index
              ? 'todo'
              : props.autoAdvance
                ? 'running'
                : 'done'
        return (
          <span
            key={slide}
            className='step'
          >
            <span
              // Remount so going back to a slide restarts its timer.
              key={`${slide}-${props.index}`}
              className={`fill ${state}`}
              data-paused={props.paused || undefined}
              onAnimationEnd={
                state === 'running' ? props.onComplete : undefined
              }
            />
          </span>
        )
      })}
    </div>
  )
}

function renderSlide(slide: Slide, { stats, nav, card }: SlideProps) {
  switch (slide) {
    case 'intro':
      return (
        <IntroSlide
          stats={stats}
          nav={nav}
          card={card}
        />
      )
    case 'blocked':
      return (
        <BlockedSlide
          stats={stats}
          nav={nav}
          card={card}
        />
      )
    case 'time':
      return (
        <TimeSlide
          stats={stats}
          nav={nav}
          card={card}
        />
      )
    case 'data':
      return (
        <DataSlide
          stats={stats}
          nav={nav}
          card={card}
        />
      )
    case 'share':
      return (
        <ShareSlide
          stats={stats}
          nav={nav}
          card={card}
        />
      )
  }
}

function Backgrounds(props: { active: Slide; paused?: boolean }) {
  // Counts how often each slide has been shown, so its photo's drift restarts
  // every time while a photo that's fading out keeps its place.
  const shown = React.useRef<Partial<Record<Slide, number>>>({})
  const lastActive = React.useRef<Slide | undefined>(undefined)
  if (lastActive.current !== props.active) {
    shown.current[props.active] = (shown.current[props.active] ?? 0) + 1
    lastActive.current = props.active
  }
  // All are mounted so they load up front and can crossfade.
  return (
    <div
      className='backgrounds'
      aria-hidden='true'
      data-paused={props.paused || undefined}
    >
      {SLIDES.map((slide) => (
        <div
          key={slide}
          className='background'
          data-slide={slide}
          data-active={slide === props.active || undefined}
        >
          <div
            key={shown.current[slide] ?? 0}
            className='background-image'
          />
        </div>
      ))}
    </div>
  )
}

function EmptyState() {
  return (
    <main className='recap-card slide-empty'>
      <Backgrounds active='intro' />
      <EmptySlide />
    </main>
  )
}

interface Props {
  stats: RecapStats
  // For stories: where to start, and share card images that resolve there.
  initialSlide?: Slide
  cardImages?: ShareCardImages
}

type Direction = 'forward' | 'back'

interface Navigation {
  index: number
  direction: Direction
  // Set once the person has moved, so the first slide doesn't animate in.
  moved: boolean
  // Slides still animating out, with the direction they left in.
  exiting: Array<{ slide: Slide; direction: Direction }>
}

type NavigationAction =
  | { type: 'step'; delta: number; animate: boolean }
  | { type: 'exited'; slide: Slide }
  | { type: 'stopMotion' }

function navigate(state: Navigation, action: NavigationAction): Navigation {
  if (action.type === 'stopMotion') {
    return state.exiting.length ? { ...state, exiting: [] } : state
  }
  if (action.type === 'exited') {
    return {
      ...state,
      exiting: state.exiting.filter((frame) => frame.slide !== action.slide),
    }
  }
  const index = Math.max(
    0,
    Math.min(state.index + action.delta, SLIDES.length - 1),
  )
  if (index === state.index) {
    return state
  }
  const direction = index > state.index ? 'forward' : 'back'
  return {
    index,
    direction,
    moved: true,
    exiting: action.animate
      ? [
          ...state.exiting.filter(
            (frame) =>
              frame.slide !== SLIDES[index]
              && frame.slide !== SLIDES[state.index],
          ),
          { slide: SLIDES[state.index], direction },
        ]
      : [],
  }
}

function Recap({
  stats,
  initialSlide = 'intro',
  cardImages = CARD_IMAGES,
}: Props) {
  const reducedMotion = useMediaQuery('(prefers-reduced-motion: reduce)')
  const [navigation, dispatch] = React.useReducer(navigate, undefined, () => ({
    index: SLIDES.indexOf(initialSlide),
    direction: 'forward' as Direction,
    moved: false,
    exiting: [],
  }))
  const [bottomBar, setBottomBar] = React.useState<HTMLElement | null>(null)
  const [holding, setHolding] = React.useState(false)
  const [holdingContent, setHoldingContent] = React.useState(false)
  const [helpOpen, setHelpOpen] = React.useState(false)
  const [keyboardFocus, setKeyboardFocus] = React.useState(false)
  const pageHidden = usePageHidden()
  const card = useShareCard(stats, cardImages)
  const pressStart = React.useRef(0)
  const lastPressMs = React.useRef(0)
  const usingKeyboard = React.useRef(false)
  const { index } = navigation
  const slide = SLIDES[index]

  const next = React.useCallback(
    () => dispatch({ type: 'step', delta: 1, animate: !reducedMotion }),
    [reducedMotion],
  )
  const back = React.useCallback(
    () => dispatch({ type: 'step', delta: -1, animate: !reducedMotion }),
    [reducedMotion],
  )

  const nav = React.useMemo<Nav>(
    () => ({
      next,
      back,
      tap: (e) => {
        if (lastPressMs.current > HOLD_MS) {
          return
        }
        const rect = e.currentTarget.getBoundingClientRect()
        const rtl = getComputedStyle(e.currentTarget).direction === 'rtl'
        const fromStart = rtl ? rect.right - e.clientX : e.clientX - rect.left
        if (fromStart < rect.width * BACK_ZONE) {
          back()
        } else {
          next()
        }
      },
      setHelpOpen,
    }),
    [next, back],
  )

  React.useEffect(() => {
    const onKeyDown = (e: KeyboardEvent) => {
      usingKeyboard.current = true
      // Leave shortcuts like Alt+Left to the browser.
      if (e.altKey || e.ctrlKey || e.metaKey) {
        return
      }
      const rtl = getComputedStyle(document.documentElement).direction === 'rtl'
      if (e.key === (rtl ? 'ArrowLeft' : 'ArrowRight')) {
        e.preventDefault()
        next()
      } else if (e.key === (rtl ? 'ArrowRight' : 'ArrowLeft')) {
        e.preventDefault()
        back()
      }
    }
    const onPointerDown = () => (usingKeyboard.current = false)
    window.addEventListener('keydown', onKeyDown)
    window.addEventListener('pointerdown', onPointerDown, true)
    return () => {
      window.removeEventListener('keydown', onKeyDown)
      window.removeEventListener('pointerdown', onPointerDown, true)
    }
  }, [next, back])

  // Slides leaving when motion is turned off never finish their transition.
  React.useEffect(() => {
    if (reducedMotion) {
      dispatch({ type: 'stopMotion' })
    }
  }, [reducedMotion])

  // Leaving the intro unmounts the tooltip without telling us it closed.
  React.useEffect(() => setHelpOpen(false), [index])

  const release = () => {
    if (holding) {
      lastPressMs.current = performance.now() - pressStart.current
    }
    setHolding(false)
    setHoldingContent(false)
  }

  // Kept in slide order so React never moves a frame, which would cancel its
  // transition.
  const frames = SLIDES.flatMap((frameSlide) => {
    if (frameSlide === slide) {
      return [
        { slide: frameSlide, exiting: false, direction: navigation.direction },
      ]
    }
    const exiting = navigation.exiting.find(
      (frame) => frame.slide === frameSlide,
    )
    return exiting ? [{ ...exiting, exiting: true }] : []
  })

  return (
    <main
      className={`recap-card slide-${slide}`}
      data-holding={holdingContent || undefined}
      style={
        {
          '--recap-hold-delay': `${HOLD_MS}ms`,
        } as React.CSSProperties
      }
      onPointerDown={(e) => {
        pressStart.current = performance.now()
        setHolding(true)
        setHoldingContent(!(e.target as Element).closest('.bottom-bar'))
      }}
      onPointerUp={release}
      onPointerCancel={release}
      onPointerLeave={release}
      onFocus={() => setKeyboardFocus(usingKeyboard.current)}
      onBlur={(e) => {
        if (!e.currentTarget.contains(e.relatedTarget as Node | null)) {
          setKeyboardFocus(false)
        }
      }}
    >
      <Backgrounds
        active={slide}
        paused={holding || helpOpen || keyboardFocus || pageHidden}
      />
      <Progress
        index={index}
        autoAdvance={!reducedMotion && !UNTIMED_SLIDES.includes(slide)}
        paused={holding || helpOpen || keyboardFocus || pageHidden}
        onComplete={next}
      />
      <div className='slides'>
        {frames.map((frame) => (
          <div
            key={frame.slide}
            className='slide-frame'
            data-motion={
              frame.exiting ? 'exit' : navigation.moved ? 'enter' : undefined
            }
            data-direction={frame.direction}
            aria-hidden={frame.exiting || undefined}
            inert={frame.exiting}
            onTransitionEnd={(e) => {
              if (frame.exiting && e.target === e.currentTarget) {
                dispatch({ type: 'exited', slide: frame.slide })
              }
            }}
          >
            <FrameContext.Provider
              value={{ exiting: frame.exiting, bottomBar }}
            >
              {renderSlide(frame.slide, { stats, nav, card })}
            </FrameContext.Provider>
          </div>
        ))}
      </div>
      <div
        className='bottom-bar'
        ref={setBottomBar}
      />
    </main>
  )
}

export function App(props: Props) {
  return (
    <div data-css-scope={style.scope}>
      <h1 className='visually-hidden'>{getLocale(S.BRAVE_RECAP_TITLE)}</h1>
      <DotMatrix />
      {hasEnoughData(props.stats) ? (
        <Recap
          stats={props.stats}
          initialSlide={props.initialSlide}
          cardImages={props.cardImages}
        />
      ) : (
        <EmptyState />
      )}
    </div>
  )
}
