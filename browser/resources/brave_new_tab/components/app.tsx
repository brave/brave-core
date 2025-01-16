/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { Background } from './background'
import { BackgroundCaption } from './background_caption'
import { SettingsModal, SettingsView } from './settings/settings_modal'

import { style } from './app.style'

export function App() {
  const [settingsView, setSettingsView] =
    React.useState<SettingsView | null>(null)

  return (
    <div data-css-scope={style.scope}>
      <button
        className='settings'
        onClick={() => setSettingsView('background')}
      >
        <Icon name='settings' />
      </button>
      <main>
        <div className='topsites-container' />
        <div className='searchbox-container' />
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
