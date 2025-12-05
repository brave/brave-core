/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { MainCard } from './main_card'
import { Footer } from './footer'

import { style } from './app.style'

type AppView =
  | 'main'
  | 'ads-blocked'
  | 'scripts-blocked'
  | 'fingerprinting-details'

export function App() {
  const [view, setView] = React.useState<AppView>('main')

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

  const inlineStyles = {
    '--app-screen-height': screen.availHeight + 'px',
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
      <Footer />
    </main>
  )
}
