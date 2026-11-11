/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import ButtonMenu from '@brave/nala/react/buttonMenu'
import Icon from '@brave/nala/react/icon'

import { TabOpenerContext } from '../../shared/components/new_tab_link'
import { EventHubContext } from '../lib/event_hub'
import { useAppState, useAppActions } from '../lib/app_context'
import { useBreakpoint } from '../lib/breakpoint'
import { RouterContext, useRoute } from '../lib/router'
import * as urls from '../../shared/lib/rewards_urls'
import * as routes from '../lib/app_routes'
import { getActiveSectionRoute, ScrollPosition } from '../lib/scroll_spy'

import { style } from './app_frame.style'

const navRoutes = [routes.home, routes.explore, routes.creators]

function getCurrentNavRoute(route: string) {
  for (const value of navRoutes) {
    if (value === route) {
      return value
    }
  }
  return routes.home
}

function scrollRouteContentIntoView(
  route: string,
  behavior: 'smooth' | 'instant',
) {
  const elem = document.querySelector(`[data-app-route="${route}"]`)
  if (!(elem instanceof HTMLElement) || !elem.offsetParent) {
    return
  }
  elem.offsetParent.scrollTo({
    top: elem.offsetTop - 16,
    left: 0,
    behavior,
  })
}

// Scrolling that follows a navigation tap must not move the highlight back.
const programmaticScrollMs = 1000
let programmaticScrollEndsAt = 0

// Keeps the current navigation item in sync with the section being scrolled
// into view.
function useScrollSpy() {
  const router = React.useContext(RouterContext)

  React.useEffect(() => {
    let frame = 0
    let timer = 0

    function update() {
      frame = 0
      const remaining = programmaticScrollEndsAt - performance.now()
      if (remaining > 0) {
        // Nothing may scroll again once the window ends, so check then.
        clearTimeout(timer)
        timer = window.setTimeout(onScroll, remaining)
        return
      }

      const elems = Array.from(
        document.querySelectorAll<HTMLElement>('[data-app-route]'),
      )
      const container = elems[0]?.offsetParent
      if (!(container instanceof HTMLElement)) {
        return
      }

      const sections = elems.map((elem) => ({
        route: elem.dataset.appRoute ?? '',
        top:
          elem.getBoundingClientRect().top
          - container.getBoundingClientRect().top,
      }))
      // Only follow the scroll while a section's own route is selected, so
      // that other routes (e.g. a modal) are left alone.
      if (!sections.some((section) => section.route === router.getRoute())) {
        return
      }

      let scrollPosition: ScrollPosition = 'none'
      if (container.scrollHeight > container.clientHeight + 1) {
        scrollPosition = 'middle'
        if (container.scrollTop <= 1) {
          scrollPosition = 'start'
        } else if (
          container.scrollTop + container.clientHeight
          >= container.scrollHeight - 2
        ) {
          scrollPosition = 'end'
        }
      }
      const route = getActiveSectionRoute(
        sections,
        container.clientHeight,
        scrollPosition,
      )
      if (route && route !== router.getRoute()) {
        // Keep the query and hash: the Explore view finds a deep-linked card
        // through the hash.
        router.replaceRoute(route + location.search + location.hash)
      }
    }

    function onScroll() {
      if (!frame) {
        frame = requestAnimationFrame(update)
      }
    }

    // Scroll events do not bubble, so listen during the capture phase.
    document.addEventListener('scroll', onScroll, true)
    return () => {
      document.removeEventListener('scroll', onScroll, true)
      cancelAnimationFrame(frame)
      clearTimeout(timer)
    }
  }, [router])
}

function NavList() {
  const { getString } = useAppActions()
  const current = getCurrentNavRoute(useRoute())
  const router = React.useContext(RouterContext)

  function onLinkClick(event: React.MouseEvent<HTMLAnchorElement>) {
    event.preventDefault()
    const route = event.currentTarget.getAttribute('href')
    if (route) {
      programmaticScrollEndsAt = performance.now() + programmaticScrollMs
      router.setRoute(route)
      scrollRouteContentIntoView(route, 'smooth')
    }
  }

  function renderLink(route: string, icon: string, text: string) {
    let className = route === current ? 'current' : ''
    return (
      <a
        className={className}
        href={route}
        onClick={onLinkClick}
      >
        <Icon name={icon} />
        <span>{text}</span>
      </a>
    )
  }

  return (
    <ul>
      <li>
        {renderLink(
          routes.home,
          'browser-home',
          getString('navigationHomeLabel'),
        )}
      </li>
      <li>
        {renderLink(
          routes.explore,
          'discover',
          getString('navigationExploreLabel'),
        )}
      </li>
      <li className='creators'>
        {renderLink(
          routes.creators,
          'star-outline',
          getString('navigationCreatorsLabel'),
        )}
      </li>
    </ul>
  )
}

function MoreMenu(props: { children: React.ReactNode }) {
  const { getString } = useAppActions()
  const tabOpener = React.useContext(TabOpenerContext)
  const eventHub = React.useContext(EventHubContext)
  const embedder = useAppState((state) => state.embedder)
  const hideReset = embedder.isBubble && embedder.platform === 'desktop'

  function onReset() {
    eventHub.dispatch('open-modal', 'reset')
  }

  function onHelp() {
    tabOpener.openTab(urls.helpURL)
  }

  return (
    <ButtonMenu className='more-menu'>
      <button slot='anchor-content'>{props.children}</button>
      <leo-menu-item onClick={onHelp}>
        <Icon name='help-outline' />
        <span>{getString('helpButtonLabel')}</span>
      </leo-menu-item>
      {!hideReset && (
        <leo-menu-item
          class='reset'
          onClick={onReset}
        >
          <Icon name='history' />
          <span>{getString('resetRewardsButtonLabel')}</span>
        </leo-menu-item>
      )}
    </ButtonMenu>
  )
}

interface Props {
  children?: React.ReactNode
}

function PanelFrame(props: Props) {
  const tabOpener = React.useContext(TabOpenerContext)
  const { getString } = useAppActions()
  const embedder = useAppState((state) => state.embedder)
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
    <div
      className='panel-frame'
      data-css-scope={style.scope}
    >
      <header className={isScrolled ? 'overlapped' : ''}>
        {embedder.platform === 'desktop' && embedder.isBubble && (
          <button
            className='expand-button'
            onClick={onExpand}
          >
            <Icon name='expand' />
          </button>
        )}
        <h4>{getString('rewardsPageTitle')}</h4>
        <MoreMenu>
          <Icon name='more-vertical' />
        </MoreMenu>
      </header>
      <main onScroll={onScroll}>{props.children}</main>
      <footer>
        <NavList />
      </footer>
    </div>
  )
}

function PageFrame(props: Props) {
  const { getString } = useAppActions()
  return (
    <div
      className='page-frame'
      data-css-scope={style.scope}
    >
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
        <main>{props.children}</main>
      </div>
    </div>
  )
}

export function AppFrame(props: Props) {
  const viewType = useBreakpoint()
  const route = useRoute()

  useScrollSpy()

  React.useEffect(() => {
    scrollRouteContentIntoView(route, 'instant')
  }, [])

  if (viewType === 'narrow') {
    return <PanelFrame {...props} />
  }

  return <PageFrame {...props} />
}
