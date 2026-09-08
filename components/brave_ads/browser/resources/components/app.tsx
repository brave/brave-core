/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { useAppState } from '../lib/app_context'
import { isWalletConnected } from '../lib/diagnostics'
import { useRoute, useRouter } from '../lib/router'
import { Rewards } from './rewards'
import { AdFormats } from './ad_formats'
import { PermissionRules } from './permission_rules'
import { Storage } from './storage'
import { UserAttention } from './user_attention'
import { Resources } from './resources'
import { Campaigns } from './campaigns'
import { DayParts } from './day_parts'
import { Segments } from './segments'
import { ConditionMatchers } from './condition_matchers'
import { ConfirmationQueue } from './confirmation_queue'
import { Conversions } from './conversions'
import { Transactions } from './transactions'
import { ConfirmationTokens } from './confirmation_tokens'
import { PaymentTokens } from './payment_tokens'
// <if expr="!is_ios">
import { Reactions } from './reactions'
// </if>
import { Events } from './events'
import { Diagnostics } from './diagnostics'
// <if expr="enable_brave_rewards && !is_ios">
import { Logs } from './logs'
// </if>
import { CopyToastProvider } from './copy_toast'
import * as routes from '../lib/app_routes'

import { style } from './app.style'

interface TabConfig {
  route: string
  label: string
  isVisible: boolean
  content: React.ReactNode
}

// Single source of truth for the sidebar list and the routed content below,
// so a tab's visibility rule can't drift between the two the way it could
// when each kept its own copy. Every entry (even one gated off, e.g. by
// Rewards not being joined) still handles its own route by falling back to
// `<Diagnostics />`, so navigating there directly never falls through to
// `NavList`'s own "not found" case instead.
function useTabs(): TabConfig[] {
  const rewardsEnabled = useAppState((state) => state.rewardsEnabled)
  const adsInternalsVerboseModeEnabled =
    useAppState((state) => state.adsInternalsVerboseModeEnabled)
  const rewardsDiagnosticEntries =
    useAppState((state) => state.rewardsDiagnosticEntries)
  // <if expr="enable_brave_rewards && !is_ios">
  const logsSupported = useAppState((state) => state.logsSupported)
  // </if>
  const walletConnected = isWalletConnected(rewardsDiagnosticEntries)
  // Permission Rules/User Attention/Transactions are all gated the same way:
  // only meaningful once Rewards is joined, and only worth the extra detail
  // once verbose mode is on.
  const verboseRewardsTabVisible =
    rewardsEnabled && adsInternalsVerboseModeEnabled
  // Confirmation Tokens/Payment Tokens are only refilled/earned once a
  // wallet is connected, not just once Rewards is joined.
  const connectedRewardsTabVisible = rewardsEnabled && walletConnected

  const tabs: TabConfig[] = [
    {
      route: routes.diagnostics,
      label: 'Diagnostics',
      isVisible: true,
      content: <Diagnostics />,
    },
    {
      route: routes.rewards,
      label: 'Rewards',
      isVisible: true,
      content: <Rewards />,
    },
    {
      route: routes.adFormats,
      label: 'Ad Formats',
      isVisible: true,
      content: <AdFormats />,
    },
    {
      route: routes.permissionRules,
      label: 'Permission Rules',
      isVisible: verboseRewardsTabVisible,
      content: verboseRewardsTabVisible
        ? <PermissionRules />
        : <Diagnostics />,
    },
    {
      route: routes.storage,
      label: 'Storage',
      isVisible: true,
      content: <Storage />,
    },
    {
      route: routes.userAttention,
      label: 'User Attention',
      isVisible: verboseRewardsTabVisible,
      content: verboseRewardsTabVisible
        ? <UserAttention />
        : <Diagnostics />,
    },
    {
      route: routes.resources,
      label: 'Resources',
      isVisible: true,
      content: <Resources />,
    },
    {
      route: routes.campaigns,
      label: 'Campaigns',
      isVisible: true,
      content: <Campaigns />,
    },
    {
      route: routes.dayParts,
      label: 'Dayparts',
      isVisible: true,
      content: <DayParts />,
    },
    {
      route: routes.segments,
      label: 'Segments',
      isVisible: true,
      content: <Segments />,
    },
    {
      route: routes.conditionMatchers,
      label: 'Condition Matchers',
      isVisible: true,
      content: <ConditionMatchers />,
    },
    {
      route: routes.confirmationQueue,
      label: 'Confirmation Queue',
      isVisible: true,
      content: <ConfirmationQueue />,
    },
    {
      route: routes.conversions,
      label: 'Conversions',
      isVisible: true,
      content: <Conversions />,
    },
    {
      route: routes.transactions,
      label: 'Transactions',
      isVisible: verboseRewardsTabVisible,
      content: verboseRewardsTabVisible
        ? <Transactions />
        : <Diagnostics />,
    },
    {
      route: routes.confirmationTokens,
      label: 'Confirmation Tokens',
      isVisible: connectedRewardsTabVisible,
      content: connectedRewardsTabVisible
        ? <ConfirmationTokens />
        : <Diagnostics />,
    },
    {
      route: routes.paymentTokens,
      label: 'Payment Tokens',
      isVisible: connectedRewardsTabVisible,
      content: connectedRewardsTabVisible
        ? <PaymentTokens />
        : <Diagnostics />,
    },
  ]

  // There's no iOS UI to generate a reaction (like/dislike an ad, etc.), so
  // this tab doesn't exist there at all.
  // <if expr="!is_ios">
  tabs.push({
    route: routes.reactions,
    label: 'Reactions',
    isVisible: rewardsEnabled,
    content: rewardsEnabled ? <Reactions /> : <Diagnostics />,
  })
  // </if>

  // Not part of the literal array above: this needs to land between the
  // platform-gated Reactions and Logs entries to preserve nav order, even
  // though it isn't itself platform-gated.
  tabs.push({
    route: routes.events,
    label: 'Events',
    isVisible: true,
    content: <Events />,
  })

  // <if expr="enable_brave_rewards && !is_ios">
  tabs.push({
    route: routes.logs,
    label: 'Logs',
    isVisible: logsSupported,
    content: logsSupported ? <Logs /> : <Diagnostics />,
  })
  // </if>

  return tabs
}

function NavList({ tabs }: { tabs: TabConfig[] }) {
  const router = useRouter()
  const currentRoute = useRoute() || routes.diagnostics

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

  return (
    <ul>
      {tabs.filter((tab) => tab.isVisible).map((tab) => (
        <li key={tab.route}>
          <a
            className={tab.route === currentRoute ? 'current' : ''}
            href={tab.route}
            onClick={onLinkClick}
          >
            <span>{tab.label}</span>
          </a>
        </li>
      ))}
    </ul>
  )
}

export function App() {
  const route = useRoute()
  const tabs = useTabs()
  const [sidebarOpen, setSidebarOpen] = React.useState(false)
  const pageContentRef = React.useRef<HTMLDivElement>(null)

  React.useEffect(() => {
    setSidebarOpen(false)
    // `.page-content` persists across tab switches, so it keeps the
    // previous tab's scroll position; a shorter tab would otherwise
    // render as already scrolled past its content.
    pageContentRef.current?.scrollTo(0, 0)
  }, [route])

  // The backdrop/close-button close paths are mouse/touch-only; this is the
  // only way a keyboard user can dismiss the drawer.
  React.useEffect(() => {
    if (!sidebarOpen) return
    const onKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'Escape') {
        setSidebarOpen(false)
      }
    }
    document.addEventListener('keydown', onKeyDown)
    return () => document.removeEventListener('keydown', onKeyDown)
  }, [sidebarOpen])

  // The open sidebar is a fixed overlay, so dragging over it rubber-bands
  // the WKWebView's document scroll; CSS containment doesn't reliably stop
  // this here, so preventDefault() on touchmove is the only guaranteed
  // fix. Skipped only while nav can still scroll away from its edge,
  // since the same unreliable containment applies there too.
  React.useEffect(() => {
    if (!sidebarOpen) return
    let lastTouchY = 0
    const onTouchStart = (e: TouchEvent) => {
      lastTouchY = e.touches[0]?.clientY ?? 0
    }
    const onTouchMove = (e: TouchEvent) => {
      const touchY = e.touches[0]?.clientY ?? lastTouchY
      const draggingDown = touchY > lastTouchY
      lastTouchY = touchY

      const target = e.target
      const nav = target instanceof Element ? target.closest('nav') : null
      if (nav && nav.scrollHeight > nav.clientHeight) {
        const atTop = nav.scrollTop <= 0
        const atBottom =
          nav.scrollTop + nav.clientHeight >= nav.scrollHeight
        if ((draggingDown && !atTop) || (!draggingDown && !atBottom)) {
          return
        }
      }
      e.preventDefault()
    }
    document.addEventListener('touchstart', onTouchStart, { passive: true })
    document.addEventListener('touchmove', onTouchMove, { passive: false })
    return () => {
      document.removeEventListener('touchstart', onTouchStart)
      document.removeEventListener('touchmove', onTouchMove)
    }
  }, [sidebarOpen])

  const content =
    tabs.find((tab) => tab.route === route)?.content ?? <Diagnostics />

  return (
    <CopyToastProvider>
      <div data-css-scope={style.scope}>
        <div className={`sidebar ${sidebarOpen ? 'open' : ''}`}>
          <header className='sidebar-close'>
            <span className='fixed-flex-item'>
              <Button
                size='small'
                kind='plain-faint'
                aria-label='Close menu'
                onClick={() => setSidebarOpen(false)}
              >
                <Icon name='hamburger-menu' />
              </Button>
            </span>
          </header>
          <nav>
            <NavList tabs={tabs} />
          </nav>
        </div>
        {sidebarOpen && (
          <div
            className='sidebar-backdrop'
            onClick={() => setSidebarOpen(false)}
          />
        )}
        <div ref={pageContentRef} className='page-content'>
          <div className='page-header'>
            <div className='sidebar-toggle'>
              <Button
                size='small'
                kind='plain-faint'
                onClick={() => setSidebarOpen(!sidebarOpen)}
              >
                <Icon name='hamburger-menu' />
              </Button>
            </div>
            <h1>Ads internals</h1>
            <div className='disclaimer'>
              WARNING: data on these pages may be sensitive. Be careful who
              you share it with.
            </div>
          </div>
          <main>
            {content}
          </main>
        </div>
      </div>
    </CopyToastProvider>
  )
}
