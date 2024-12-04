/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { SearchBox } from './search/search_box'
import { Background } from './background'
import { BackgroundCaption } from './background_caption'
import { SettingsModal, SettingsView } from './settings/settings_modal'
import { TopSites } from './top_sites/top_sites'
import { Clock } from './clock'

import { style } from './app.style'

export function App() {
  const [settingsView, setSettingsView] =
    React.useState<SettingsView | null>(null)

  return (
    <div {...style}>
      <div className='top-controls'>
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
      </div>
      <main>
        <div className='topsites-container'>
          <TopSites />
        </div>
        <div className='searchbox-container'>
          <SearchBox
            onCustomizeSearchEngineList={() => setSettingsView('search')}
          />
        </div>
        <div className='background-caption-container'>
          <BackgroundCaption />
        </div>
        <div className='widget-container' />
      </main>
      <Background />
      <SettingsModal
        isOpen={Boolean(settingsView)}
        initialView={settingsView}
        onClose={() => setSettingsView(null)}
      />
    </div>
  )
}
