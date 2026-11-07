// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'
import { RecapStats, hasEnoughData } from '../stats'
import '../strings'

function PrivacyNote() {
  return (
    <p className='privacy-note'>
      <Icon name='shield-done' />
      {getLocale(S.BRAVE_RECAP_PRIVACY_NOTE)}
    </p>
  )
}

function IntroSlide() {
  return (
    <>
      <div className='logo'>
        <Icon name='brave-icon-release-color' />
      </div>
      <h1>{getLocale(S.BRAVE_RECAP_INTRO_HEADING)}</h1>
      <p className='lead'>{getLocale(S.BRAVE_RECAP_INTRO_NO_DATE)}</p>
      <PrivacyNote />
    </>
  )
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
  if (!hasEnoughData(stats)) {
    return <EmptyState />
  }

  return (
    <div className='recap slide-intro'>
      <div className='slide'>
        <IntroSlide />
      </div>
    </div>
  )
}
