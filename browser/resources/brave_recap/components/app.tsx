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
  RecapStats,
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
} from '../format'
import '../strings'

const SLIDES = ['intro', 'blocked', 'time', 'data'] as const
type Slide = (typeof SLIDES)[number]

const COUNT_UP_MS = 1400

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
      {getLocale(S.BRAVE_RECAP_PRIVACY_NOTE)}
    </p>
  )
}

function IntroSlide({ stats }: { stats: RecapStats }) {
  const days = getDaysSinceFirstRun(stats)
  return (
    <>
      <div className='logo'>
        <Icon name='brave-icon-release-color' />
      </div>
      <h1>{getLocale(S.BRAVE_RECAP_INTRO_HEADING)}</h1>
      {stats.firstRunTime ? (
        <>
          <p className='lead'>
            {formatLocale(S.BRAVE_RECAP_INTRO_SINCE, {
              $1: formatMonthYear(stats.firstRunTime),
            })}
          </p>
          {days > 0 && (
            <p>
              {formatLocale(S.BRAVE_RECAP_INTRO_DAYS, {
                $1: formatCount(days),
              })}
            </p>
          )}
        </>
      ) : (
        <p className='lead'>{getLocale(S.BRAVE_RECAP_INTRO_NO_DATE)}</p>
      )}
      <PrivacyNote />
    </>
  )
}

function BlockedSlide({ stats }: { stats: RecapStats }) {
  const value = useCountUp(stats.adsBlocked)
  return (
    <>
      <div className='hero'>
        <BigValue
          value={{ value: formatCount(value), unit: '' }}
          final={{ value: formatCount(stats.adsBlocked), unit: '' }}
        />
        <h2>{getLocale(S.BRAVE_RECAP_BLOCKED_HEADING)}</h2>
      </div>
      <p>{getLocale(S.BRAVE_RECAP_BLOCKED_TEXT)}</p>
    </>
  )
}

function TimeSlide({ stats }: { stats: RecapStats }) {
  const value = useCountUp(getTimeSavedMs(stats))
  const movies = getMoviesEquivalent(stats)
  return (
    <>
      <div className='hero'>
        <BigValue
          value={formatTimeSaved(value)}
          final={formatTimeSaved(getTimeSavedMs(stats))}
        />
        <h2>{getLocale(S.BRAVE_RECAP_TIME_HEADING)}</h2>
      </div>
      <p>
        {movies > 1
          ? formatLocale(S.BRAVE_RECAP_TIME_TEXT, { $1: formatCount(movies) })
          : getLocale(
              movies === 1
                ? S.BRAVE_RECAP_TIME_TEXT_ONE
                : S.BRAVE_RECAP_TIME_TEXT_SHORT,
            )}
      </p>
    </>
  )
}

function DataSlide({ stats }: { stats: RecapStats }) {
  const value = useCountUp(stats.bandwidthSavedBytes)
  const hours = getHdVideoHoursEquivalent(stats)
  return (
    <>
      <div className='hero'>
        <BigValue
          value={formatBandwidth(value)}
          final={formatBandwidth(stats.bandwidthSavedBytes)}
        />
        <h2>{getLocale(S.BRAVE_RECAP_DATA_HEADING)}</h2>
      </div>
      <p>
        {hours > 1
          ? formatLocale(S.BRAVE_RECAP_DATA_TEXT, { $1: formatCount(hours) })
          : getLocale(
              hours === 1
                ? S.BRAVE_RECAP_DATA_TEXT_ONE
                : S.BRAVE_RECAP_DATA_TEXT_SHORT,
            )}
      </p>
    </>
  )
}

function renderSlide(slide: Slide, stats: RecapStats) {
  switch (slide) {
    case 'intro':
      return <IntroSlide stats={stats} />
    case 'blocked':
      return <BlockedSlide stats={stats} />
    case 'time':
      return <TimeSlide stats={stats} />
    case 'data':
      return <DataSlide stats={stats} />
  }
}

function EmptyState() {
  return (
    <div className='recap slide-intro'>
      <div className='slide'>
        <div className='logo'>
          <Icon name='brave-icon-release-color' />
        </div>
        <h1>{getLocale(S.BRAVE_RECAP_EMPTY_HEADING)}</h1>
        <p className='lead'>{getLocale(S.BRAVE_RECAP_EMPTY_TEXT)}</p>
      </div>
    </div>
  )
}

export function App({ stats }: { stats: RecapStats }) {
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
    <div className={`recap slide-${slide}`}>
      <div className='progress'>
        <Navdots
          dotCount={SLIDES.length}
          activeDot={index}
          onChange={(e) => setIndex(e.activeDot)}
          label={getLocale(S.BRAVE_RECAP_SLIDES_LABEL)}
          getDotLabel={(dot) =>
            formatLocale(S.BRAVE_RECAP_SLIDE_LABEL, {
              $1: formatCount(dot + 1),
              $2: formatCount(SLIDES.length),
            })
          }
        />
      </div>
      <div
        className='slide'
        key={slide}
        onClick={next}
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
          {getLocale(S.BRAVE_RECAP_BACK)}
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
            {getLocale(S.BRAVE_RECAP_START_OVER)}
          </Button>
        ) : (
          <Button
            kind='filled'
            size='large'
            onClick={next}
          >
            {getLocale(S.BRAVE_RECAP_NEXT)}
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
