/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { SearchBox } from './search/search_box'
import { Background } from './background/background'
import { BackgroundClickRegion } from './background/background_click_region'
import { BackgroundCaption } from './background/background_caption'
import { SettingsModal, SettingsView } from './settings/settings_modal'
import { TopSites } from './top_sites/top_sites'
import { Clock } from './common/clock'
import { LazyNewsFeed } from './news/lazy_news_feed'
import { WidgetStack } from './widgets/widget_stack'
import { useSearchLayoutReady, useWidgetLayoutReady } from './app_layout_ready'
import useMediaQuery from '$web-common/useMediaQuery'

import { style, threeColumnBreakpoint } from './app.style'

const threeColumnQuery = `(width > ${threeColumnBreakpoint})`

export function App() {
  const searchLayoutReady = useSearchLayoutReady()
  const widgetLayoutReady = useWidgetLayoutReady()

  const [settingsView, setSettingsView] = React.useState<SettingsView | null>(
    null,
  )

  const threeColumnWidth = useMediaQuery(threeColumnQuery)

  React.useEffect(() => {
    const params = new URLSearchParams(location.search)
    const settingsArg = params.get('openSettings')
    if (settingsArg === null) {
      return
    }
    setSettingsView(settingsArg === 'BraveNews' ? 'news' : 'background')
    history.pushState(null, '', '/')
  }, [])

  return (
    <div data-css-scope={style.scope}>
      <Background />
      <div className='background-filter allow-background-pointer-events' />
      <main className='allow-background-pointer-events'>
        <button
          className='clock'
          onClick={() => setSettingsView('clock')}
        >
          <Clock />
        </button>
        <button
          className='settings'
          onClick={() => setSettingsView('background')}
        >
          <Icon name='settings' />
        </button>
        <div className='topsites-container'>
          <TopSites />
        </div>
        <div className='searchbox-container'>
          {searchLayoutReady && (
            <SearchBox showSearchSettings={() => setSettingsView('search')} />
          )}
        </div>
        <div
          className='
          spacer
          sponsored-background-safe-area
          allow-background-pointer-events'
        >
          <BackgroundClickRegion />
        </div>
        <div className='caption-container'>
          <BackgroundCaption />
        </div>
        <div className='widget-container'>
          {widgetLayoutReady && (
            <>
              {threeColumnWidth ? (
                <>
                  <WidgetStack
                    name='left'
                    tabs={['stats']}
                  />
                  <WidgetStack
                    name='center'
                    tabs={['news']}
                  />
                </>
              ) : (
                <WidgetStack
                  name='left'
                  tabs={['stats', 'news']}
                />
              )}
              <WidgetStack
                name='right'
                tabs={['vpn', 'rewards', 'talk']}
              />
            </>
          )}
        </div>
      </main>
      <div className='news-container'>
        <LazyNewsFeed />
      </div>
      <SettingsModal
        isOpen={settingsView !== null}
        initialView={settingsView}
        onClose={() => setSettingsView(null)}
      />
    </div>
  )
}
