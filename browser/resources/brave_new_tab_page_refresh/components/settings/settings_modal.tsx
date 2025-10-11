/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Dialog from '@brave/leo/react/dialog'
import Icon from '@brave/leo/react/icon'
import Navigation from '@brave/leo/react/navigation'
import NavigationItem from '@brave/leo/react/navigationItem'

import { useBraveNews } from '../../../../../components/brave_news/browser/resources/shared/Context'

import { useNewTabState } from '../../context/new_tab_context'
import { useSearchState } from '../../context/search_context'
import { BackgroundPanel } from './background_panel'
import { SearchPanel } from './search_panel'
import { TopSitesPanel } from './top_sites_panel'
import { ClockPanel } from './clock_panel'
import { WidgetsPanel } from './widgets_panel'
import { getString } from '../../lib/strings'

import { style } from './settings_modal.style'

export type SettingsView =
  'background' |
  'search' |
  'top-sites' |
  'news' |
  'clock' |
  'widgets'

interface Props {
  initialView: SettingsView | null
  isOpen: boolean
  onClose: () => void
}

export function SettingsModal(props: Props) {
  const braveNews = useBraveNews()
  const searchFeatureEnabled = useSearchState((s) => s.searchFeatureEnabled)
  const newsFeatureEnabled = useNewTabState((s) => s.newsFeatureEnabled)

  const [currentView, setCurrentView] =
      React.useState<SettingsView>(props.initialView || 'background')

  React.useEffect(() => {
    if (props.isOpen) {
      if (props.initialView === 'news') {
        braveNews.setCustomizePage('news')
        setCurrentView('background')
      } else {
        setCurrentView(props.initialView ?? 'background')
      }
    }
  }, [props.isOpen, props.initialView])

  function shouldShowView(view: SettingsView) {
    switch (view) {
      case 'search': return searchFeatureEnabled
      case 'news': return newsFeatureEnabled
      default: return true
    }
  }

  function renderPanel() {
    if (!shouldShowView(currentView)) {
      return null
    }
    switch (currentView) {
      case 'background': return <BackgroundPanel />
      case 'search': return <SearchPanel />
      case 'top-sites': return <TopSitesPanel />
      case 'news': return null
      case 'clock': return <ClockPanel />
      case 'widgets': return <WidgetsPanel />
    }
  }

  function getNavItemText(view: SettingsView) {
    switch (view) {
      case 'background': return getString(S.NEW_TAB_BACKGROUND_SETTINGS_TITLE)
      case 'search': return getString(S.NEW_TAB_SEARCH_SETTINGS_TITLE)
      case 'top-sites': return getString(S.NEW_TAB_TOP_SITES_SETTINGS_TITLE)
      case 'news': return getString(S.BRAVE_NEWS_SETTINGS_TITLE)
      case 'clock': return getString(S.NEW_TAB_CLOCK_SETTINGS_TITLE)
      case 'widgets': return getString(S.NEW_TAB_WIDGET_SETTINGS_TITLE)
    }
  }

  function getNavItemIcon(view: SettingsView) {
    switch (view) {
      case 'background': return <Icon name='image' />
      case 'search': return <Icon name='search' />
      case 'top-sites': return <Icon name='window-content' />
      case 'news': return <Icon name='product-brave-news' />
      case 'clock': return <Icon name='clock' />
      case 'widgets': return <Icon name='browser-ntp-widget' />
    }
  }

  function renderNavItem(view: SettingsView) {
    if (!shouldShowView(view)) {
      return null
    }
    return (
      <NavigationItem
        isCurrent={view === currentView}
        onClick={() => {
          if (view === 'news') {
            braveNews.setCustomizePage('news')
          } else {
            setCurrentView(view)
          }
        }}
      >
        {getNavItemIcon(view)}
        {getNavItemText(view)}
      </NavigationItem>
    )
  }

  return (
    <div data-css-scope={style.scope}>
      <Dialog
        isOpen={props.isOpen}
        showClose
        onClose={() => props.onClose()}
        backdropClickCloses={!braveNews.customizePage}
      >
        <h3>
          {getString(S.NEW_TAB_SETTINGS_TITLE)}
        </h3>
        <div className='panel-body'>
          <nav>
            <Navigation>
              {renderNavItem('background')}
              {renderNavItem('search')}
              {renderNavItem('top-sites')}
              {renderNavItem('news')}
              {renderNavItem('clock')}
              {renderNavItem('widgets')}
            </Navigation>
          </nav>
          <section>
            <div>{renderPanel()}</div>
          </section>
        </div>
      </Dialog>
    </div>
  )
}
