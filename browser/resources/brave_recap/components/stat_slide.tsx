// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale } from '$web-common/locale'
import { getTimeSavedMs } from '../stats'
import {
  BLOCKED_COMPARISONS,
  Comparison,
  DATA_COMPARISONS,
  TIME_COMPARISONS,
  pickComparison,
} from '../comparisons'
import { formatBandwidth, formatCount, formatTimeSaved } from '../format'
import { prefersReducedMotion } from './hooks'
import {
  BackButton,
  Nav,
  NextButton,
  SlideLayout,
  SlideProps,
} from './slide_layout'
import { Panels } from './panels'
import { style } from './stat_slide.style'
import '../strings'

const COUNT_UP_MS = 1400

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

// Reserves the final value's width so the number doesn't shift while counting.
// The final value is what screen readers get; the counting one is hidden.
function BigValue(props: {
  to: number
  format: (value: number) => string
  before?: string
  after?: string
}) {
  const final = props.format(props.to)
  return (
    <span className='big-value'>
      {props.before && <span className='big-unit'>{props.before}</span>}
      <span
        className='big-number'
        style={{ '--chars': final.length } as React.CSSProperties}
      >
        <span className='sizer'>{final}</span>
        <span aria-hidden='true'>
          <CountUp
            to={props.to}
            format={props.format}
          />
        </span>
      </span>
      {props.after && <span className='big-unit'>{props.after}</span>}
    </span>
  )
}

function StatSlide(props: {
  nav: Nav
  to: number
  format: (value: number) => string
  // The final value's unit, kept with the number so it agrees with it.
  unit?: { before: string; after: string }
  caption: string
  comparisons: Comparison[]
  finalValue: number
  text: string
}) {
  const comparison = pickComparison(props.comparisons, props.finalValue)
  return (
    <SlideLayout
      scope={style.scope}
      onContentClick={props.nav.tap}
      start={<BackButton nav={props.nav} />}
      primary={<NextButton nav={props.nav} />}
    >
      <h2 className='stat'>
        <BigValue
          to={props.to}
          format={props.format}
          before={props.unit?.before}
          after={props.unit?.after}
        />
        {props.caption}
      </h2>
      <Panels>
        {comparison && (
          <p className='panel comparison'>{getLocale(comparison)}</p>
        )}
        <p className='panel note'>{props.text}</p>
      </Panels>
    </SlideLayout>
  )
}

export function BlockedSlide({ stats, nav }: SlideProps) {
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

export function TimeSlide({ stats, nav }: SlideProps) {
  const timeSaved = getTimeSavedMs(stats)
  return (
    <StatSlide
      nav={nav}
      to={timeSaved}
      // Keeps the final value's unit while counting.
      format={(value) => formatTimeSaved(value, timeSaved).value}
      unit={formatTimeSaved(timeSaved)}
      caption={getLocale(S.BRAVE_RECAP_TIME_HEADING)}
      comparisons={TIME_COMPARISONS}
      finalValue={timeSaved}
      text={getLocale(S.BRAVE_RECAP_TIME_TEXT)}
    />
  )
}

export function DataSlide({ stats, nav }: SlideProps) {
  const bytes = stats.bandwidthSavedBytes
  return (
    <StatSlide
      nav={nav}
      to={bytes}
      format={(value) => formatBandwidth(value, bytes).value}
      unit={formatBandwidth(bytes)}
      caption={getLocale(S.BRAVE_RECAP_DATA_HEADING)}
      comparisons={DATA_COMPARISONS}
      finalValue={bytes}
      text={getLocale(S.BRAVE_RECAP_DATA_TEXT)}
    />
  )
}
