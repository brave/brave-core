/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Dialog from '@brave/leo/react/dialog'
import Navigation from '@brave/leo/react/navigation'
import NavigationItem from '@brave/leo/react/navigationItem'

import { BackgroundPanel } from './background_panel'
import { TopSitesPanel } from './top_sites_panel'
import { SearchPanel } from './search_panel'
import { ClockPanel } from './clock_panel'
import { StatsPanel } from './stats_panel'
import { useLocale } from '../locale_context'

import { style } from './settings_modal.style'

export type SettingsView =
  'background' |
  'top-sites' |
  'search' |
  'clock' |
  'stats'

interface Props {
  initialView: SettingsView | null
  isOpen: boolean
  onClose: () => void
}

export function SettingsModal(props: Props) {
  const { getString } = useLocale()

  const [currentView, setCurrentView] =
    React.useState<SettingsView>(props.initialView || 'background')

  React.useEffect(() => {
    if (props.isOpen) {
      setCurrentView(props.initialView || 'background')
    }
  }, [props.isOpen, props.initialView])

  function renderPanel() {
    switch (currentView) {
      case 'background': return <BackgroundPanel />
      case 'top-sites': return <TopSitesPanel />
      case 'search': return <SearchPanel />
      case 'clock': return <ClockPanel />
      case 'stats': return <StatsPanel />
    }
  }

  function getNavItemText(view: SettingsView) {
    switch (view) {
      case 'background': return getString('backgroundSettingsTitle')
      case 'top-sites': return getString('topSitesSettingsTitle')
      case 'search': return getString('searchSettingsTitle')
      case 'clock': return getString('clockSettingsTitle')
      case 'stats': return getString('statsSettingsTitle')
    }
  }

  function renderNavItem(view: SettingsView) {
    return (
      <NavigationItem
        isCurrent={view === currentView}
        onClick={() => setCurrentView(view)}
      >
        {getNavItemText(view)}
      </NavigationItem>
    )
  }

  return (
    <div data-css-scope={style.scope}>
      <Dialog isOpen={props.isOpen} showClose onClose={props.onClose}>
        <h3>
          {getString('settingsTitle')}
        </h3>
        <div className='panel-body'>
          <nav>
            <Navigation>
              {renderNavItem('background')}
              {renderNavItem('search')}
              {renderNavItem('stats')}
              {renderNavItem('top-sites')}
              {renderNavItem('clock')}
            </Navigation>
          </nav>
          <section>
            {renderPanel()}
          </section>
        </div>
      </Dialog>
    </div>
  )
}
