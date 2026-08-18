/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { useAppState } from '../lib/app_context'
import { useRoute, useRouter } from '../lib/router'
import { Conversions } from './conversions'
import { Events } from './events'
import { Diagnostics } from './diagnostics'
// <if expr="enable_brave_rewards && !is_ios">
import { Logs } from './logs'
// </if>
import { ClearAdsDataButton } from './clear_ads_data_button'
import * as routes from '../lib/app_routes'

import { style } from './app.style'

function NavList() {
  const router = useRouter()
  const currentRoute = useRoute() || routes.diagnostics
  const logsSupported = useAppState((state) => state.logsSupported)

  function onLinkClick(event: React.MouseEvent<HTMLAnchorElement>) {
    if (event.defaultPrevented || event.button !== 0 ||
        event.metaKey || event.ctrlKey || event.shiftKey || event.altKey) {
      return
    }
    event.preventDefault()
    const route = event.currentTarget.getAttribute('href')
    if (route) {
      router.setRoute(route)
    }
  }

  function renderLink(route: string, text: string) {
    const className = route === currentRoute ? 'current' : ''
    return (
      <a
        className={className}
        href={route}
        onClick={onLinkClick}
      >
        <span>{text}</span>
      </a>
    )
  }

  return (
    <ul>
      <li>{renderLink(routes.diagnostics, 'Diagnostics')}</li>
      <li>{renderLink(routes.conversions, 'Conversions')}</li>
      <li>{renderLink(routes.events, 'Events')}</li>
      {logsSupported && <li>{renderLink(routes.logs, 'Logs')}</li>}
    </ul>
  )
}

export function App() {
  const route = useRoute()
  // <if expr="enable_brave_rewards && !is_ios">
  const logsSupported = useAppState((state) => state.logsSupported)
  // </if>
  const [sidebarOpen, setSidebarOpen] = React.useState(false)

  React.useEffect(() => {
    setSidebarOpen(false)
  }, [route])

  function renderContent() {
    switch (route) {
      case routes.conversions:
        return <Conversions />
      case routes.events:
        return <Events />
      case routes.logs:
        // <if expr="enable_brave_rewards && !is_ios">
        if (logsSupported) {
          return <Logs />
        }
        // </if>
        return <Diagnostics />
      default:
        return <Diagnostics />
    }
  }

  function maybeCloseSidebar(event: React.UIEvent) {
    const isToggleClick =
      event.target instanceof HTMLElement
      && event.target.closest('.sidebar-toggle')
    if (!isToggleClick) {
      setSidebarOpen(false)
    }
  }

  return (
    <div data-css-scope={style.scope}>
      <div className={`sidebar ${sidebarOpen ? 'open' : ''}`}>
        <header />
        <nav>
          <NavList />
        </nav>
      </div>
      <div
        className='page-content'
        onClick={maybeCloseSidebar}
        onKeyDown={maybeCloseSidebar}
      >
        <div className='sidebar-toggle'>
          <Button
            size='small'
            kind='plain-faint'
            onClick={() => setSidebarOpen(!sidebarOpen)}
          >
            <Icon name='hamburger-menu' />
          </Button>
        </div>
        <main>
          <h1>Ads internals</h1>
          <div className='disclaimer'>
            WARNING: data on these pages may be sensitive. Be careful who you
            share it with.
          </div>
          <div className='header-actions'>
            <ClearAdsDataButton />
          </div>
          {renderContent()}
        </main>
      </div>
    </div>
  )
}
