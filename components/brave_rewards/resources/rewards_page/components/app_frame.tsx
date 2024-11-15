/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import Icon from '@brave/leo/react/icon'

import { TabOpenerContext } from '../../shared/components/new_tab_link'
import { EventHubContext } from '../lib/event_hub'
import { useAppState } from '../lib/app_model_context'
import { useLocaleContext } from '../lib/locale_strings'
import { useBreakpoint } from '../lib/breakpoint'
import { RouterContext, useRoute } from '../lib/router'
import * as urls from '../../shared/lib/rewards_urls'
import * as routes from '../lib/app_routes'

import { style } from './app_frame.style'

const navRoutes = [
  routes.home,
  routes.explore,
  routes.creators
]

function getCurrentNavRoute(route: string) {
  for (const value of navRoutes) {
    if (value === route) {
      return value
    }
  }
  return routes.home
}

function NavList() {
  const { getString } = useLocaleContext()
  const current = getCurrentNavRoute(useRoute())
  const router = React.useContext(RouterContext)

  function onLinkClick(event: React.MouseEvent<HTMLAnchorElement>) {
    event.preventDefault()
    router.setRoute(event.currentTarget.getAttribute('href') ?? '')
  }

  function renderLink(route: string, icon: string, text: string) {
    let className = route === current ? 'current' : ''
    return (
      <a className={className} href={route} onClick={onLinkClick}>
        <Icon name={icon} />
        <span>{text}</span>
      </a>
    )
  }

  return (
    <ul>
      <li>
        {
          renderLink(
            routes.home,
            'browser-home',
            getString('navigationHomeLabel'))
        }
      </li>
      <li>
        {
          renderLink(
            routes.explore,
            'discover',
            getString('navigationExploreLabel'))
        }
      </li>
      <li className='creators'>
        {
          renderLink(
            routes.creators,
            'star-outline',
            getString('navigationCreatorsLabel'))
        }
      </li>
    </ul>
  )
}

function MoreMenu(props: { children: React.ReactNode }) {
  const { getString } = useLocaleContext()
  const tabOpener = React.useContext(TabOpenerContext)
  const eventHub = React.useContext(EventHubContext)
  const isBubble = useAppState((state) => state.embedder.isBubble)

  function onReset() {
    if (isBubble) {
      tabOpener.openTab(urls.resetURL)
      return
    }
    eventHub.dispatch('open-modal', 'reset')
  }

  function onHelp() {
    tabOpener.openTab(urls.helpURL)
  }

  return (
    <ButtonMenu className='more-menu'>
      <button slot='anchor-content'>
        {props.children}
      </button>
      <leo-menu-item onClick={onHelp}>
        <Icon name='help-outline' />
        <span>{getString('helpButtonLabel')}</span>
      </leo-menu-item>
      {
        !isBubble &&
          <leo-menu-item class='reset' onClick={onReset}>
            <Icon name='history' />
            <span>{getString('resetRewardsButtonLabel')}</span>
          </leo-menu-item>
      }
    </ButtonMenu>
  )
}

interface Props {
  children?: React.ReactNode
}

function PanelFrame(props: Props) {
  const tabOpener = React.useContext(TabOpenerContext)
  const { getString } = useLocaleContext()
  const [isScrolled, setIsScrolled] = React.useState(false)

  function onExpand() {
    tabOpener.openTab(urls.settingsURL)
  }

  function onScroll(event: React.UIEvent) {
    const { scrollTop } = event.currentTarget
    if (scrollTop === 0) {
      setIsScrolled(false)
    } else if (!isScrolled) {
      setIsScrolled(true)
    }
  }

  return (
    <div className='panel-frame' {...style}>
      <header className={isScrolled ? 'overlapped' : ''}>
        <button className='expand-button' onClick={onExpand}>
          <Icon name='expand' />
        </button>
        <h4>{getString('rewardsPageTitle')}</h4>
        <MoreMenu><Icon name='more-vertical' /></MoreMenu>
      </header>
      <main onScroll={onScroll}>
        {props.children}
      </main>
      <footer>
        <NavList />
      </footer>
    </div>
  )
}

function PageFrame(props: Props) {
  const { getString } = useLocaleContext()
  return (
    <div className='page-frame' {...style}>
      <div className='sidebar'>
        <header>
          <div className='brave-rewards-logo' />
        </header>
        <nav>
          <NavList />
        </nav>
        <footer>
          <ul>
            <li>
              <MoreMenu>
                <Icon name='more-horizontal' />
                <span>{getString('moreButtonLabel')}</span>
              </MoreMenu>
            </li>
          </ul>
        </footer>
      </div>
      <div className='page-content'>
        <main>
          {props.children}
        </main>
      </div>
    </div>
  )
}

export function AppFrame(props: Props) {
  const viewType = useBreakpoint()

  if (viewType === 'narrow') {
    return <PanelFrame {...props} />
  }

  return <PageFrame {...props} />
}
