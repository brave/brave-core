/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useShieldsState, useShieldsActions } from '../lib/shields_context'
import { MainCard } from './main_card'
import { Footer } from './footer'
import { AdvancedSettingsToggle } from './advanced_settings_toggle'
import { AdvancedSettings } from './advanced_settings'
import { AdBlockOnlyNotice } from './adblock_only_notice'
import { AdsBlockedDetails } from './ads_blocked_details'
import { FingerprintingDetails } from './fingerprinting_details'
import { ScriptsBlockedDetails } from './scripts_blocked_details'
import { AdblockOnlyPrompt } from './adblock_only_prompt'

import { style } from './app.style'

type AppView =
  | 'main'
  | 'ads-blocked'
  | 'scripts-blocked'
  | 'fingerprinting-details'

export function App() {
  const actions = useShieldsActions()
  const browserWindowHeight = useShieldsState((s) => s.browserWindowHeight)
  const initialized = useShieldsState((s) => s.initialized)
  const [view, setView] = React.useState<AppView>('main')
  const onBack = () => setView('main')

  React.useEffect(() => {
    if (initialized) {
      actions.notifyAppReady()
    } else {
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

  if (!initialized) {
    return (
      <div
        data-css-scope={style.scope}
        className='app-loading'
      />
    )
  }

  const inlineStyles = {
    '--browser-window-height': Math.max(browserWindowHeight, 600) + 'px',
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
      <AdvancedSettingsToggle />
      <AdvancedSettings
        showAdsBlocked={() => props.showView('ads-blocked')}
        showScriptsBlocked={() => props.showView('scripts-blocked')}
        showFingerprintingDetails={() =>
          props.showView('fingerprinting-details')
        }
      />
      <AdBlockOnlyNotice />
      <AdblockOnlyPrompt />
      <Footer />
    </main>
  )
}
