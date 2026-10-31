// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import Navdots from '@brave/leo/react/navdots'
import { getLocale, formatLocale } from '$web-common/locale'
import {
  WrappedStats,
  getDaysSinceFirstRun,
  getHdVideoHoursEquivalent,
  getMoviesEquivalent,
  getTimeSavedMs,
  hasEnoughData,
} from '../stats'
import {
  FormattedValue,
  formatBandwidth,
  formatCount,
  formatMonthYear,
  formatTimeSaved,
  joinValue,
} from '../format'
import { renderShareCard } from '../share_card'
import '../strings'

// TODO: replace with an attributed download link once growth provides one.
const DOWNLOAD_URL = 'https://brave.com/download/'
// Same-origin so it doesn't taint the share card canvas.
const LOGO_URL = '/brave-icon.svg'

const SLIDES = ['intro', 'blocked', 'time', 'data', 'share'] as const
type Slide = (typeof SLIDES)[number]

const COUNT_UP_MS = 1400

// Animates from 0 to `target` when the component mounts.
function useCountUp(target: number) {
  const [value, setValue] = React.useState(0)
  React.useEffect(() => {
    if (window.matchMedia('(prefers-reduced-motion: reduce)').matches) {
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

function ValueText({ value }: { value: FormattedValue }) {
  return (
    <>
      {value.value}
      {value.unit && <span className='unit'>{value.unit}</span>}
    </>
  )
}

// Reserves the final value's width so the card doesn't grow while counting.
function BigValue(props: { value: FormattedValue; final: FormattedValue }) {
  return (
    <div className='big-value'>
      <span
        className='sizer'
        aria-hidden='true'
      >
        <ValueText value={props.final} />
      </span>
      <span>
        <ValueText value={props.value} />
      </span>
    </div>
  )
}

function PrivacyNote() {
  return (
    <p className='privacy-note'>
      <Icon name='shield-done' />
      {getLocale(S.BRAVE_WRAPPED_PRIVACY_NOTE)}
    </p>
  )
}

function IntroSlide({ stats }: { stats: WrappedStats }) {
  const days = getDaysSinceFirstRun(stats)
  return (
    <>
      <div className='logo'>
        <Icon name='brave-icon-release-color' />
      </div>
      <h1>{getLocale(S.BRAVE_WRAPPED_INTRO_HEADING)}</h1>
      {stats.firstRunTime ? (
        <>
          <p className='lead'>
            {formatLocale(S.BRAVE_WRAPPED_INTRO_SINCE, {
              $1: formatMonthYear(stats.firstRunTime),
            })}
          </p>
          {days > 0 && (
            <p>
              {formatLocale(S.BRAVE_WRAPPED_INTRO_DAYS, {
                $1: formatCount(days),
              })}
            </p>
          )}
        </>
      ) : (
        <p className='lead'>{getLocale(S.BRAVE_WRAPPED_INTRO_NO_DATE)}</p>
      )}
      <PrivacyNote />
    </>
  )
}

function BlockedSlide({ stats }: { stats: WrappedStats }) {
  const value = useCountUp(stats.adsBlocked)
  return (
    <>
      <div className='hero'>
        <BigValue
          value={{ value: formatCount(value), unit: '' }}
          final={{ value: formatCount(stats.adsBlocked), unit: '' }}
        />
        <h2>{getLocale(S.BRAVE_WRAPPED_BLOCKED_HEADING)}</h2>
      </div>
      <p>{getLocale(S.BRAVE_WRAPPED_BLOCKED_TEXT)}</p>
    </>
  )
}

function TimeSlide({ stats }: { stats: WrappedStats }) {
  const value = useCountUp(getTimeSavedMs(stats))
  const movies = getMoviesEquivalent(stats)
  return (
    <>
      <div className='hero'>
        <BigValue
          value={formatTimeSaved(value)}
          final={formatTimeSaved(getTimeSavedMs(stats))}
        />
        <h2>{getLocale(S.BRAVE_WRAPPED_TIME_HEADING)}</h2>
      </div>
      <p>
        {movies > 1
          ? formatLocale(S.BRAVE_WRAPPED_TIME_TEXT, { $1: formatCount(movies) })
          : getLocale(
              movies === 1
                ? S.BRAVE_WRAPPED_TIME_TEXT_ONE
                : S.BRAVE_WRAPPED_TIME_TEXT_SHORT,
            )}
      </p>
    </>
  )
}

function DataSlide({ stats }: { stats: WrappedStats }) {
  const value = useCountUp(stats.bandwidthSavedBytes)
  const hours = getHdVideoHoursEquivalent(stats)
  return (
    <>
      <div className='hero'>
        <BigValue
          value={formatBandwidth(value)}
          final={formatBandwidth(stats.bandwidthSavedBytes)}
        />
        <h2>{getLocale(S.BRAVE_WRAPPED_DATA_HEADING)}</h2>
      </div>
      <p>
        {hours > 1
          ? formatLocale(S.BRAVE_WRAPPED_DATA_TEXT, { $1: formatCount(hours) })
          : getLocale(
              hours === 1
                ? S.BRAVE_WRAPPED_DATA_TEXT_ONE
                : S.BRAVE_WRAPPED_DATA_TEXT_SHORT,
            )}
      </p>
    </>
  )
}

function getShareText(stats: WrappedStats) {
  const text = formatLocale(S.BRAVE_WRAPPED_SHARE_POST_TEXT, {
    $1: formatCount(stats.adsBlocked),
    $2: joinValue(formatTimeSaved(getTimeSavedMs(stats))),
    $3: joinValue(formatBandwidth(stats.bandwidthSavedBytes)),
  })
  return `${text} ${DOWNLOAD_URL}`
}

function openShareIntent(url: string) {
  window.open(url, '_blank', 'noopener')
}

function ShareSlide({ stats }: { stats: WrappedStats }) {
  const [card, setCard] = React.useState<Blob>()
  const [cardUrl, setCardUrl] = React.useState<string>()
  const [copied, setCopied] = React.useState<'image' | 'text'>()

  React.useEffect(() => {
    let url: string | undefined
    let cancelled = false
    renderShareCard(
      {
        title: getLocale(S.BRAVE_WRAPPED_CARD_TITLE),
        subtitle: stats.firstRunTime
          ? formatLocale(S.BRAVE_WRAPPED_CARD_SINCE, {
              $1: formatMonthYear(stats.firstRunTime),
            })
          : '',
        stats: [
          {
            value: formatCount(stats.adsBlocked),
            label: getLocale(S.BRAVE_WRAPPED_BLOCKED_HEADING),
          },
          {
            value: joinValue(formatTimeSaved(getTimeSavedMs(stats))),
            label: getLocale(S.BRAVE_WRAPPED_TIME_HEADING),
          },
          {
            value: joinValue(formatBandwidth(stats.bandwidthSavedBytes)),
            label: getLocale(S.BRAVE_WRAPPED_DATA_HEADING),
          },
        ],
        cta: getLocale(S.BRAVE_WRAPPED_CARD_CTA),
      },
      LOGO_URL,
    ).then((blob) => {
      if (cancelled) {
        return
      }
      url = URL.createObjectURL(blob)
      setCard(blob)
      setCardUrl(url)
    })
    return () => {
      cancelled = true
      if (url) {
        URL.revokeObjectURL(url)
      }
    }
  }, [stats])

  const flashCopied = (what: 'image' | 'text') => {
    setCopied(what)
    setTimeout(() => setCopied(undefined), 2000)
  }

  const copyImage = async () => {
    if (!card) {
      return
    }
    await navigator.clipboard.write([new ClipboardItem({ 'image/png': card })])
    flashCopied('image')
  }

  const download = () => {
    if (!cardUrl) {
      return
    }
    const link = document.createElement('a')
    link.href = cardUrl
    link.download = 'brave-wrapped.png'
    link.click()
  }

  const shareText = getShareText(stats)
  const encodedText = encodeURIComponent(shareText)

  return (
    <div className='share'>
      <div className='card-preview'>
        {cardUrl && (
          <img
            src={cardUrl}
            alt={getLocale(S.BRAVE_WRAPPED_CARD_TITLE)}
          />
        )}
      </div>
      <div className='share-actions'>
        <h1>{getLocale(S.BRAVE_WRAPPED_SHARE_HEADING)}</h1>
        <p>{getLocale(S.BRAVE_WRAPPED_SHARE_TEXT)}</p>
        <div className='button-row'>
          <Button
            kind='filled'
            size='large'
            onClick={copyImage}
            isDisabled={!card}
          >
            <Icon
              slot='icon-before'
              name={copied === 'image' ? 'check-normal' : 'copy'}
            />
            {getLocale(
              copied === 'image'
                ? S.BRAVE_WRAPPED_COPIED
                : S.BRAVE_WRAPPED_COPY_IMAGE,
            )}
          </Button>
          <Button
            kind='outline'
            size='large'
            onClick={download}
            isDisabled={!cardUrl}
          >
            <Icon
              slot='icon-before'
              name='download'
            />
            {getLocale(S.BRAVE_WRAPPED_DOWNLOAD)}
          </Button>
        </div>
        <div className='button-row'>
          <Button
            kind='outline'
            onClick={() =>
              openShareIntent(`https://x.com/intent/post?text=${encodedText}`)
            }
          >
            <Icon
              slot='icon-before'
              name='social-x-color'
            />
            {getLocale(S.BRAVE_WRAPPED_POST_ON_X)}
          </Button>
          <Button
            kind='outline'
            onClick={() =>
              openShareIntent(
                `https://www.reddit.com/submit?url=${encodeURIComponent(
                  DOWNLOAD_URL,
                )}&title=${encodeURIComponent(
                  formatLocale(S.BRAVE_WRAPPED_SHARE_POST_TITLE, {
                    $1: formatCount(stats.adsBlocked),
                  }),
                )}`,
              )
            }
          >
            <Icon
              slot='icon-before'
              name='social-reddit-color'
            />
            {getLocale(S.BRAVE_WRAPPED_POST_ON_REDDIT)}
          </Button>
          <Button
            kind='plain'
            onClick={async () => {
              await navigator.clipboard.writeText(shareText)
              flashCopied('text')
            }}
          >
            <Icon
              slot='icon-before'
              name={copied === 'text' ? 'check-normal' : 'link-normal'}
            />
            {getLocale(
              copied === 'text'
                ? S.BRAVE_WRAPPED_COPIED
                : S.BRAVE_WRAPPED_COPY_LINK,
            )}
          </Button>
        </div>
        <PrivacyNote />
      </div>
    </div>
  )
}

function renderSlide(slide: Slide, stats: WrappedStats) {
  switch (slide) {
    case 'intro':
      return <IntroSlide stats={stats} />
    case 'blocked':
      return <BlockedSlide stats={stats} />
    case 'time':
      return <TimeSlide stats={stats} />
    case 'data':
      return <DataSlide stats={stats} />
    case 'share':
      return <ShareSlide stats={stats} />
  }
}

function EmptyState() {
  return (
    <div className='wrapped slide-intro'>
      <div className='slide'>
        <div className='logo'>
          <Icon name='brave-icon-release-color' />
        </div>
        <h1>{getLocale(S.BRAVE_WRAPPED_EMPTY_HEADING)}</h1>
        <p className='lead'>{getLocale(S.BRAVE_WRAPPED_EMPTY_TEXT)}</p>
      </div>
    </div>
  )
}

export function App({ stats }: { stats: WrappedStats }) {
  const [index, setIndex] = React.useState(0)
  const slide = SLIDES[index]
  const isLast = index === SLIDES.length - 1

  const next = React.useCallback(
    () => setIndex((i) => Math.min(i + 1, SLIDES.length - 1)),
    [],
  )
  const back = React.useCallback(() => setIndex((i) => Math.max(i - 1, 0)), [])

  React.useEffect(() => {
    const onKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'ArrowRight' || e.key === ' ') {
        e.preventDefault()
        next()
      } else if (e.key === 'ArrowLeft') {
        e.preventDefault()
        back()
      }
    }
    window.addEventListener('keydown', onKeyDown)
    return () => window.removeEventListener('keydown', onKeyDown)
  }, [next, back])

  if (!hasEnoughData(stats)) {
    return <EmptyState />
  }

  return (
    <div className={`wrapped slide-${slide}`}>
      <div className='progress'>
        <Navdots
          dotCount={SLIDES.length}
          activeDot={index}
          onChange={(e) => setIndex(e.activeDot)}
          label={getLocale(S.BRAVE_WRAPPED_SLIDES_LABEL)}
          getDotLabel={(dot) =>
            formatLocale(S.BRAVE_WRAPPED_SLIDE_LABEL, {
              $1: formatCount(dot + 1),
              $2: formatCount(SLIDES.length),
            })
          }
        />
      </div>
      <div
        className='slide'
        key={slide}
        onClick={slide === 'share' ? undefined : next}
      >
        {renderSlide(slide, stats)}
      </div>
      <div className='nav'>
        <Button
          kind='plain'
          size='large'
          onClick={back}
          isDisabled={index === 0}
        >
          <Icon
            slot='icon-before'
            name='arrow-left'
          />
          {getLocale(S.BRAVE_WRAPPED_BACK)}
        </Button>
        {isLast ? (
          <Button
            kind='plain'
            size='large'
            onClick={() => setIndex(0)}
          >
            <Icon
              slot='icon-before'
              name='refresh'
            />
            {getLocale(S.BRAVE_WRAPPED_START_OVER)}
          </Button>
        ) : (
          <Button
            kind='filled'
            size='large'
            onClick={next}
          >
            {getLocale(S.BRAVE_WRAPPED_NEXT)}
            <Icon
              slot='icon-after'
              name='arrow-right'
            />
          </Button>
        )}
      </div>
    </div>
  )
}
