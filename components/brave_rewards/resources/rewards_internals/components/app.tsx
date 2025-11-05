/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { useLocale } from '../lib/app_model_context'
import { useRoute, useRouter } from '../../rewards_page/lib/router'
import { GeneralInfo } from './general_info'
import { RewardsLog } from './rewards_log'
import { Contributions } from './contributions'
import { RewardsEvents } from './rewards_events'
import { AdDiagnostics } from './ad_diagnostics'
import * as routes from '../lib/app_routes'

import { style } from './app.style'

function NavList() {
  const router = useRouter()
  const currentRoute = useRoute() || routes.home

  function onLinkClick(event: React.MouseEvent<HTMLAnchorElement>) {
    event.preventDefault()
    const route = event.currentTarget.getAttribute('href')
    if (route) {
      router.setRoute(route)
    }
  }

  function renderLink(route: string, text: string) {
    let className = route === currentRoute ? 'current' : ''
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
      <li>{renderLink(routes.home, 'General Info')}</li>
      <li>{renderLink(routes.rewardsLog, 'Rewards Log')}</li>
      <li>{renderLink(routes.contributions, 'Contributions')}</li>
      <li>{renderLink(routes.rewardsEvents, 'Events')}</li>
      <li>{renderLink(routes.adDiagnostics, 'Ad Diagnostics')}</li>
    </ul>
  )
}

export function App() {
  const { getString } = useLocale()
  const route = useRoute()
  const [sidebarOpen, setSidebarOpen] = React.useState(false)

  React.useEffect(() => {
    setSidebarOpen(false)
  }, [route])

  function renderContent() {
    switch (route) {
      case routes.rewardsLog:
        return <RewardsLog />
      case routes.contributions:
        return <Contributions />
      case routes.rewardsEvents:
        return <RewardsEvents />
      case routes.adDiagnostics:
        return <AdDiagnostics />
      default:
        return <GeneralInfo />
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
        <header>
          <div className='brave-rewards-logo' />
        </header>
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
          <h1>{getString('pageTitle')}</h1>
          <div className='disclaimer'>{getString('pageDisclaimerText')}</div>
          {renderContent()}
        </main>
      </div>
    </div>
  )
}
