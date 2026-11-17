// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/nala/react/button'
import Icon from '@brave/nala/react/icon'
import Tooltip from '@brave/nala/react/tooltip'
import { getLocale, formatLocale } from '$web-common/locale'
import { getDaysSinceFirstRun } from '../stats'
import { formatCount, formatMonthYear } from '../format'
import { SlideLayout, SlideProps } from './slide_layout'
import '../strings'

function AppIcon() {
  return (
    <div className='app-icon'>
      <Icon name='brave-icon-release-color' />
    </div>
  )
}

export function IntroSlide({ stats, nav }: SlideProps) {
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
        <h2>{getLocale(S.BRAVE_RECAP_INTRO_HEADING)}</h2>
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

export function EmptySlide() {
  return (
    <div className='slide'>
      <div className='intro'>
        <AppIcon />
        <h2>{getLocale(S.BRAVE_RECAP_EMPTY_HEADING)}</h2>
      </div>
      <div className='panels'>
        <p className='panel note'>{getLocale(S.BRAVE_RECAP_EMPTY_TEXT)}</p>
      </div>
    </div>
  )
}
