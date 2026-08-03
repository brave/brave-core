/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { getLocale } from '$web-common/locale'
import * as React from 'react'
import { useBraveNews } from '../shared/Context'
import DisabledPlaceholder from './DisabledPlaceholder'
import Discover from './Discover'
import NewsSettings from './NewsSettings'
import { PopularPage } from './Popular'
import SourcesList from './SourcesList'
import { SuggestionsPage } from './Suggestions'

import { style } from './Configure.style'

export default function Configure() {
  const {
    customizePage,
    isOptInPrefEnabled,
    isShowOnNTPPrefEnabled,
  } = useBraveNews()

  // TODO(petemill): We'll probably need to have 2 toggles, or some other
  // way to know if brave news is "enabled" when Brave News is exposed
  // in places other than just the NTP. For now this is pretty tied to NTP.
  const isBraveNewsFullyEnabled = isOptInPrefEnabled && isShowOnNTPPrefEnabled

  let content: JSX.Element
  if (!isBraveNewsFullyEnabled) {
    content = <DisabledPlaceholder />
  } else if (customizePage === 'suggestions') {
    content = <SuggestionsPage />
  } else if (customizePage === 'popular') {
    content = <PopularPage />
  } else {
    content = <Discover />
  }

  return (
    <div data-css-scope={style.scope} id='brave-news-configure'>
      <div className='panel-body'>
        <nav>
          <h4>{getLocale(S.BRAVE_NEWS_SETTINGS_TITLE)}</h4>
          <SourcesList />
          {!isBraveNewsFullyEnabled && <div className='nav-overlay' />}
        </nav>
        <section>
          <div>
            <NewsSettings />
            {content}
          </div>
        </section>
      </div>
    </div>
  )
}
