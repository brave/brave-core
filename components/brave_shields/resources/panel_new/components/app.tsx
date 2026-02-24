/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { MainCard } from './main_card'
import { Footer } from './footer'
import { AdvancedSettingsHeader } from './advanced_settings_header'
import { useInitializedStatus } from './use_initialized_status'
import { useCacheInvalidator } from './use_cache_invalidator'

import { style } from './app.style'

type AppView =
  | 'main'
  | 'ads-blocked'
  | 'scripts-blocked'
  | 'fingerprinting-details'

export function App() {
  useCacheInvalidator()

  const initialized = useInitializedStatus()
  const [view, setView] = React.useState<AppView>('main')

  React.useEffect(() => {
    if (initialized) {
      setView('main')
    }
  }, [initialized])

  function renderContent() {
    switch (view) {
      case 'main':
        return <MainView showView={setView} />
      case 'ads-blocked':
        return null
      case 'scripts-blocked':
        return null
      case 'fingerprinting-details':
        return null
    }
  }

  if (!initialized) {
    return (
      <div
        data-css-scope={style.scope}
        className='app-loading'
      />
    )
  }

  return <div data-css-scope={style.scope}>{renderContent()}</div>
}

function MainView(props: { showView: (view: AppView) => void }) {
  return (
    <main>
      <MainCard />
      <AdvancedSettingsHeader />
      <Footer />
    </main>
  )
}
