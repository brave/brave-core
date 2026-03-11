/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useShieldsApi } from '../api/shields_api_context'
import { MainCard } from './main_card'
import { Footer } from './footer'
import { AdvancedSettingsHeader } from './advanced_settings_header'
import { AdvancedSettings } from './advanced_settings'
import { AdsBlockedDetails } from './ads_blocked_details'
import { FingerprintingDetails } from './fingerprinting_details'
import { ScriptsBlockedDetails } from './scripts_blocked_details'
import { MaybeAdBlockOnlyPrompt } from './ad_block_only_prompt'
import { MaybeAdBlockOnlyNotice } from './ad_block_only_notice'
import { useCacheInvalidator } from './use_cache_invalidator'
import { useInitializedStatus } from './use_initialized_status'

import { style } from './app.style'

type AppView =
  | 'main'
  | 'ads-blocked'
  | 'scripts-blocked'
  | 'fingerprinting-details'

export function App() {
  useCacheInvalidator()

  const api = useShieldsApi()
  const { data: browserWindowHeight } = api.useGetBrowserWindowHeight()
  const initialized = useInitializedStatus()
  const [view, setView] = React.useState<AppView>('main')

  const onBack = () => setView('main')

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
        return <AdsBlockedDetails onBack={onBack} />
      case 'scripts-blocked':
        return <ScriptsBlockedDetails onBack={onBack} />
      case 'fingerprinting-details':
        return <FingerprintingDetails onBack={onBack} />
    }
  }

  const inlineStyles = {
    '--browser-window-height': Math.max(browserWindowHeight ?? 0, 600) + 'px',
  }

  return (
    <div
      data-css-scope={style.scope}
      style={inlineStyles as React.CSSProperties}
    >
      {renderContent()}
    </div>
  )
}

function MainView(props: { showView: (view: AppView) => void }) {
  return (
    <main>
      <MainCard />
      <AdvancedSettingsHeader />
      <AdvancedSettings
        showAdsBlocked={() => props.showView('ads-blocked')}
        showScriptsBlocked={() => props.showView('scripts-blocked')}
        showFingerprintingDetails={() =>
          props.showView('fingerprinting-details')
        }
      />
      <MaybeAdBlockOnlyNotice />
      <MaybeAdBlockOnlyPrompt />
      <Footer />
    </main>
  )
}
