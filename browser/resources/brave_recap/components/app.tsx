// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { createPortal } from 'react-dom'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import Tooltip from '@brave/leo/react/tooltip'
import { getLocale, formatLocale } from '$web-common/locale'
import {
  RecapStats,
  getDaysSinceFirstRun,
  getTimeSavedMs,
  hasEnoughData,
} from '../stats'
import {
  BLOCKED_COMPARISONS,
  Comparison,
  DATA_COMPARISONS,
  TIME_COMPARISONS,
  pickComparison,
} from '../comparisons'
import {
  formatBandwidth,
  formatCount,
  formatMonthYear,
  formatTimeSaved,
  joinValue,
} from '../format'
import { ShareCardImages, renderShareCard } from '../share_card'
import '../strings'

// TODO: replace with an attributed download link once growth provides one.
const DOWNLOAD_URL = 'https://brave.com/download/'
const RECAP_URL = 'brave://recap'
// Same-origin so they don't taint the share card canvas.
const CARD_IMAGES: ShareCardImages = {
  logo: '/brave-icon.svg',
  pattern: '/images/card_pattern.png',
}

export const SLIDES = ['intro', 'blocked', 'time', 'data', 'share'] as const
export type Slide = (typeof SLIDES)[number]
// These wait for the person: the intro until they start, the share slide
// because it's the end.
const UNTIMED_SLIDES: readonly Slide[] = ['intro', 'share']

const COUNT_UP_MS = 1400
// A press longer than this pauses the slide rather than advancing it.
const HOLD_MS = 300
// How much of the slide, from the start side, goes back when tapped.
const BACK_ZONE = 1 / 3

function prefersReducedMotion() {
  return window.matchMedia('(prefers-reduced-motion: reduce)').matches
}

function useCountUp(target: number) {
  const [value, setValue] = React.useState(0)
  React.useEffect(() => {
    if (prefersReducedMotion()) {
      setValue(target)
      return
    }
    let frame = 0
    const start = performance.now()
    const tick = (now: number) => {
      const t = Math.min(1, (now - start) / COUNT_UP_MS)
      const eased = 1 - Math.pow(1 - t, 3)
      setValue(target * eased)
      if (t < 1) {
        frame = requestAnimationFrame(tick)
      }
    }
    frame = requestAnimationFrame(tick)
    return () => cancelAnimationFrame(frame)
  }, [target])
  return value
}

// Only this text re-renders while the number counts up.
function CountUp(props: { to: number; format: (value: number) => string }) {
  return <>{props.format(useCountUp(props.to))}</>
}

function usePageHidden() {
  const [hidden, setHidden] = React.useState(document.hidden)
  React.useEffect(() => {
    const onChange = () => setHidden(document.hidden)
    document.addEventListener('visibilitychange', onChange)
    return () => document.removeEventListener('visibilitychange', onChange)
  }, [])
  return hidden
}

interface Nav {
  next: () => void
  back: () => void
  // A tap on the start third of the slide goes back and anywhere else
  // advances, like stories elsewhere. Nothing happens at the end of a long
  // press.
  tap: (e: React.MouseEvent<HTMLElement>) => void
  setHelpOpen: (open: boolean) => void
}

interface SlideProps {
  stats: RecapStats
  nav: Nav
  // The share image, rendered when the recap opens so it's ready on arrival.
  card?: ShareCard
}

interface ShareCard {
  blob: Blob
  url: string
}

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

function BackButton({ nav }: { nav: Nav }) {
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

function NextButton({ nav }: { nav: Nav }) {
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

// Each slide renders inside a frame. Only the current frame owns the shared
// bottom bar, so the buttons stay put while slide content moves.
interface Frame {
  exiting: boolean
  bottomBar: HTMLElement | null
}

const FrameContext = React.createContext<Frame>({
  exiting: false,
  bottomBar: null,
})

function SlideLayout(props: {
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

function AppIcon() {
  return (
    <div className='app-icon'>
      <Icon name='brave-icon-release-color' />
    </div>
  )
}

function IntroSlide({ stats, nav }: SlideProps) {
  const days = getDaysSinceFirstRun(stats)
  return (
    <SlideLayout
      onContentClick={nav.tap}
      start={
        <Tooltip
          mode='default'
          placement='top-start'
          onVisibilityChange={({ visible }) => nav.setHelpOpen(visible)}
        >
          <Button
            fab
            kind='filled'
            size='medium'
            aria-label={getLocale(S.BRAVE_RECAP_HELP_LABEL)}
            aria-description={getLocale(S.BRAVE_RECAP_PRIVACY_NOTE)}
            data-testid='recap-help'
          >
            <Icon name='help-outline' />
          </Button>
          <div
            slot='content'
            className='help'
          >
            {getLocale(S.BRAVE_RECAP_PRIVACY_NOTE)}
          </div>
        </Tooltip>
      }
      primary={
        <Button
          kind='filled'
          size='medium'
          onClick={nav.next}
        >
          {getLocale(S.BRAVE_RECAP_GET_STARTED)}
          <Icon
            slot='icon-after'
            name='carat-right'
          />
        </Button>
      }
    >
      <div className='intro'>
        <AppIcon />
        <h1>{getLocale(S.BRAVE_RECAP_INTRO_HEADING)}</h1>
      </div>
      <div className='messages'>
        <p className='message start'>
          {stats.firstRunTime
            ? formatLocale(S.BRAVE_RECAP_INTRO_SINCE, {
                $1: <strong>{formatMonthYear(stats.firstRunTime)}</strong>,
              })
            : getLocale(S.BRAVE_RECAP_INTRO_NO_DATE)}
        </p>
        {days > 1 && (
          <p className='message end'>
            {formatLocale(S.BRAVE_RECAP_INTRO_DAYS, {
              $1: <strong>{formatCount(days)}</strong>,
            })}
          </p>
        )}
      </div>
    </SlideLayout>
  )
}

// Reserves the final value's width so the number doesn't shift while counting.
// The final value is what screen readers get; the counting one is hidden.
function BigValue(props: { to: number; format: (value: number) => string }) {
  const final = props.format(props.to)
  return (
    <div
      className='big-value'
      style={{ '--chars': final.length } as React.CSSProperties}
    >
      <span className='sizer'>{final}</span>
      <span aria-hidden='true'>
        <CountUp
          to={props.to}
          format={props.format}
        />
      </span>
    </div>
  )
}

function StatSlide(props: {
  nav: Nav
  to: number
  format: (value: number) => string
  caption: string
  comparisons: Comparison[]
  finalValue: number
  text: string
}) {
  const comparison = pickComparison(props.comparisons, props.finalValue)
  return (
    <SlideLayout
      onContentClick={props.nav.tap}
      start={<BackButton nav={props.nav} />}
      primary={<NextButton nav={props.nav} />}
    >
      <div className='stat'>
        <BigValue
          to={props.to}
          format={props.format}
        />
        <h2>{props.caption}</h2>
      </div>
      <div className='panels'>
        {comparison && (
          <p className='panel comparison'>{getLocale(comparison)}</p>
        )}
        <p className='panel note'>{props.text}</p>
      </div>
    </SlideLayout>
  )
}

function BlockedSlide({ stats, nav }: SlideProps) {
  return (
    <StatSlide
      nav={nav}
      to={stats.adsBlocked}
      format={formatCount}
      caption={getLocale(S.BRAVE_RECAP_BLOCKED_HEADING)}
      comparisons={BLOCKED_COMPARISONS}
      finalValue={stats.adsBlocked}
      text={getLocale(S.BRAVE_RECAP_BLOCKED_TEXT)}
    />
  )
}

function TimeSlide({ stats, nav }: SlideProps) {
  const timeSaved = getTimeSavedMs(stats)
  const final = formatTimeSaved(timeSaved)
  return (
    <StatSlide
      nav={nav}
      to={timeSaved}
      // Keeps the final value's unit while counting.
      format={(value) => formatTimeSaved(value, timeSaved).value}
      caption={formatLocale(S.BRAVE_RECAP_TIME_CAPTION, { $1: final.unit })}
      comparisons={TIME_COMPARISONS}
      finalValue={timeSaved}
      text={getLocale(S.BRAVE_RECAP_TIME_TEXT)}
    />
  )
}

function DataSlide({ stats, nav }: SlideProps) {
  const bytes = stats.bandwidthSavedBytes
  const final = formatBandwidth(bytes)
  return (
    <StatSlide
      nav={nav}
      to={bytes}
      format={(value) => formatBandwidth(value, bytes).value}
      caption={formatLocale(S.BRAVE_RECAP_DATA_CAPTION, { $1: final.unit })}
      comparisons={DATA_COMPARISONS}
      finalValue={bytes}
      text={getLocale(S.BRAVE_RECAP_DATA_TEXT)}
    />
  )
}

function getShareText(stats: RecapStats) {
  const text = formatLocale(S.BRAVE_RECAP_SHARE_POST_TEXT, {
    $1: formatCount(stats.adsBlocked),
    $2: joinValue(formatTimeSaved(getTimeSavedMs(stats))),
    $3: joinValue(formatBandwidth(stats.bandwidthSavedBytes)),
  })
  return `${text} ${DOWNLOAD_URL}`
}

function openShareIntent(url: string) {
  window.open(url, '_blank', 'noopener')
}

function useShareCard(stats: RecapStats, images: ShareCardImages) {
  const [card, setCard] = React.useState<ShareCard>()
  React.useEffect(() => {
    let url: string | undefined
    let cancelled = false
    renderShareCard(
      {
        title: getLocale(S.BRAVE_RECAP_CARD_TITLE),
        subtitle: stats.firstRunTime
          ? formatLocale(S.BRAVE_RECAP_CARD_SINCE, {
              $1: formatMonthYear(stats.firstRunTime),
            })
          : '',
        stats: [
          {
            value: formatCount(stats.adsBlocked),
            label: getLocale(S.BRAVE_RECAP_BLOCKED_HEADING),
          },
          {
            value: joinValue(formatTimeSaved(getTimeSavedMs(stats))),
            label: getLocale(S.BRAVE_RECAP_TIME_HEADING),
          },
          {
            value: joinValue(formatBandwidth(stats.bandwidthSavedBytes)),
            label: getLocale(S.BRAVE_RECAP_DATA_HEADING),
          },
        ],
        cta: getLocale(S.BRAVE_RECAP_CARD_CTA),
        url: RECAP_URL,
      },
      images,
    )
      .then((blob) => {
        if (cancelled) {
          return
        }
        url = URL.createObjectURL(blob)
        setCard({ blob, url })
      })
      .catch((e) => {
        console.error('[brave-recap] failed to render share card:', e)
      })
    return () => {
      cancelled = true
      if (url) {
        URL.revokeObjectURL(url)
      }
    }
  }, [stats, images])
  return card
}

// Neither X nor Reddit can take an image through a share link, so the image is
// put on the clipboard to paste into the post.
async function copyImage(card: ShareCard) {
  try {
    await navigator.clipboard.write([
      new ClipboardItem({ [card.blob.type || 'image/png']: card.blob }),
    ])
    return true
  } catch (e) {
    console.error('[brave-recap] failed to copy the share image:', e)
    return false
  }
}

function ShareSlide({ stats, nav, card }: SlideProps) {
  const [imageCopied, setImageCopied] = React.useState(false)

  const download = () => {
    if (!card) {
      return
    }
    const link = document.createElement('a')
    link.href = card.url
    link.download = 'brave-recap.png'
    link.click()
  }

  // Copy first: the post opens in a new tab, and the clipboard can only be
  // written while this page has focus.
  const share = async (url: string) => {
    if (card && (await copyImage(card))) {
      setImageCopied(true)
    }
    openShareIntent(url)
  }

  const shareText = encodeURIComponent(getShareText(stats))
  const redditTitle = encodeURIComponent(
    formatLocale(S.BRAVE_RECAP_SHARE_POST_TITLE, {
      $1: formatCount(stats.adsBlocked),
    }),
  )

  return (
    <SlideLayout
      start={<BackButton nav={nav} />}
      primary={
        <Button
          kind='filled'
          size='medium'
          onClick={download}
          isDisabled={!card}
        >
          <Icon
            slot='icon-before'
            name='download'
          />
          {getLocale(S.BRAVE_RECAP_DOWNLOAD)}
        </Button>
      }
    >
      <div className='share'>
        <div className='card-preview'>
          {card && (
            <img
              src={card.url}
              alt={getLocale(S.BRAVE_RECAP_CARD_TITLE)}
            />
          )}
        </div>
        <div className='share-actions'>
          <Button
            kind='outline'
            size='medium'
            onClick={() => share(`https://x.com/intent/post?text=${shareText}`)}
          >
            <Icon
              slot='icon-before'
              name='social-x-color'
            />
            {getLocale(S.BRAVE_RECAP_POST_ON_X)}
          </Button>
          <Button
            kind='outline'
            size='medium'
            onClick={() =>
              // A text post, so the image can be pasted next to the same text
              // as on X.
              share(
                `https://www.reddit.com/submit?selftext=true&title=${redditTitle}&text=${shareText}`,
              )
            }
          >
            <Icon
              slot='icon-before'
              name='social-reddit-color'
            />
            {getLocale(S.BRAVE_RECAP_POST_ON_REDDIT)}
          </Button>
          {imageCopied && (
            <p
              className='share-hint'
              role='status'
            >
              {getLocale(S.BRAVE_RECAP_IMAGE_COPIED)}
            </p>
          )}
        </div>
      </div>
    </SlideLayout>
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
      <div className='slide'>
        <div className='intro'>
          <AppIcon />
          <h1>{getLocale(S.BRAVE_RECAP_EMPTY_HEADING)}</h1>
        </div>
        <div className='panels'>
          <p className='panel note'>{getLocale(S.BRAVE_RECAP_EMPTY_TEXT)}</p>
        </div>
      </div>
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

function navigate(state: Navigation, action: NavigationAction): Navigation {
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
  const [reducedMotion] = React.useState(prefersReducedMotion)
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
      if (e.key === 'ArrowRight') {
        e.preventDefault()
        next()
      } else if (e.key === 'ArrowLeft') {
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

  // Leaving the intro unmounts the tooltip without telling us it closed.
  React.useEffect(() => setHelpOpen(false), [index])

  const release = () => {
    if (holding) {
      lastPressMs.current = performance.now() - pressStart.current
    }
    setHolding(false)
    setHoldingContent(false)
  }

  // Kept in slide order so each slide keeps its React state while it leaves.
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
    <div className='stage'>
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
