/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { Background } from '../background/background'
import { TopSites } from '../top_sites/top_sites'
import { AskInput } from './ask_input'
import { Widgets } from './widgets'
import { Sidebar } from './sidebar'

import { style } from './ask_app.style'

export function AskApp() {
  const [sidebarOpen, setSidebarOpen] = React.useState(false)

  return (
    <div data-css-scope={style.scope}>
      <Background />
      <div className='background-filter allow-background-pointer-events' />
      <main className='allow-background-pointer-events'>
        <button
          className='sidebar'
          onClick={() => setSidebarOpen(true)}
        >
          <Icon name='window-tabs-vertical-expanded' />
        </button>
        <button
          className='settings'
          onClick={() => {}}
        >
          <Icon name='settings' />
        </button>
        <div className='topsites-container'>
          <TopSites collapsedTileColumnCount={10} />
        </div>
        <div className='inputbox-container'>
          <AskInput />
        </div>
        <div className='widget-container'>
          <Widgets />
        </div>
      </main>
      <Sidebar isOpen={sidebarOpen} onClose={() => setSidebarOpen(false)} />
    </div>
  )
}

export default AskApp
