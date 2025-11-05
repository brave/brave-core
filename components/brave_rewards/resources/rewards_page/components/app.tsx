/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import ProgressRing from '@brave/leo/react/progressRing'

import { AppModelContext, useAppState } from '../lib/app_model_context'
import { EventHubContext } from '../lib/event_hub'
import { useRoute } from '../lib/router'
import { useBreakpoint } from '../lib/breakpoint'
import { AppErrorBoundary } from './app_error_boundary'
import { AppFrame } from './app_frame'
import { HomeView } from './home/home_view'
import { ExploreView } from './explore/explore_view'
import { Onboarding } from './onboarding/onboarding'
import { OnboardingSuccessModal } from './onboarding/onboarding_success_modal'
import { ConnectAccount } from './connect/connect_account'
import { UnsupportedRegionView } from './unsupported_region_view'
import { AuthorizationModal } from './connect/authorization_modal'
import { CaptchaModal } from './modals/captcha_modal'
import { ContributeModal } from './contribute/contribute_modal'
import { ResetModal } from './modals/reset_modal'
import { TosUpdateModal } from './modals/tos_update_modal'
import { NotificationModal } from './modals/notification_modal'
import { SelfCustodyInviteModal } from './modals/self_custody_invite_modal'
import { useShouldShowSelfCustodyInvite } from '../lib/self_custody_invite'
import { useConnectAccountRouter } from '../lib/connect_account_router'
import * as routes from '../lib/app_routes'

import { style } from './app.style'

export function App() {
  const model = React.useContext(AppModelContext)
  const eventHub = React.useContext(EventHubContext)

  const loading = useAppState((state) => state.loading)
  const isUnsupportedRegion = useAppState((state) => state.isUnsupportedRegion)
  const embedder = useAppState((state) => state.embedder)
  const paymentId = useAppState((state) => state.paymentId)
  const tosUpdateRequired = useAppState((state) => state.tosUpdateRequired)
  const captchaInfo = useAppState((state) => state.captchaInfo)
  const notifications = useAppState((state) => state.notifications)

  const viewType = useBreakpoint()

  const [showResetModal, setShowResetModal] = React.useState(false)
  const [showContributeModal, setShowContributeModal] = React.useState(false)
  const [hideCaptcha, setHideCaptcha] = React.useState(false)
  const [showOnboardingSuccess, setShowOnboardingSuccess] =
    React.useState(false)

  const route = useRoute((route, router) => {
    if (route === routes.reset) {
      setShowResetModal(true)
      router.replaceRoute('/')
    }
  })

  const shouldShowSelfCustodyInvite = useShouldShowSelfCustodyInvite()
  const connectAccount = useConnectAccountRouter()

  React.useEffect(() => {
    return eventHub.addListener('open-modal', (data) => {
      switch (data) {
        case 'reset':
          setShowResetModal(true)
          break
        case 'contribute':
          setShowContributeModal(true)
          break
      }
    })
  }, [eventHub])

  const onMount = React.useCallback((elem: HTMLElement | null) => {
    if (elem) {
      elem.style.setProperty(
        '--app-screen-height',
        window.screen.availHeight + 'px',
      )
    }
  }, [])

  function getClassNames() {
    const list: string[] = []
    if (embedder.isAutoResizeBubble) {
      list.push('is-auto-resize-bubble')
    }
    if (embedder.isBubble) {
      list.push('is-bubble')
    }
    if (viewType === 'narrow') {
      list.push('is-narrow-view')
    } else {
      list.push('is-wide-view')
    }
    if (embedder.animatedBackgroundEnabled) {
      list.push('animated-background')
    }
    return list.join(' ')
  }

  function renderModal() {
    if (route.endsWith(routes.authorization)) {
      return <AuthorizationModal />
    }

    if (showOnboardingSuccess) {
      return (
        <OnboardingSuccessModal
          onClose={() => setShowOnboardingSuccess(false)}
        />
      )
    }

    if (showResetModal) {
      const onReset = () => {
        model.resetRewards()
        setShowResetModal(false)
      }
      return (
        <ResetModal
          onReset={onReset}
          onClose={() => setShowResetModal(false)}
        />
      )
    }

    if (showContributeModal) {
      return <ContributeModal onClose={() => setShowContributeModal(false)} />
    }

    if (captchaInfo && !hideCaptcha) {
      return (
        <CaptchaModal
          captchaInfo={captchaInfo}
          onCaptchaResult={(success) => model.onCaptchaResult(success)}
          onClose={() => setHideCaptcha(true)}
        />
      )
    }

    if (tosUpdateRequired) {
      return (
        <TosUpdateModal
          onAccept={() => model.acceptTermsOfServiceUpdate()}
          onReset={() => setShowResetModal(true)}
        />
      )
    }

    if (notifications.length > 0) {
      return <NotificationModal notification={notifications[0]} />
    }

    if (shouldShowSelfCustodyInvite) {
      return (
        <SelfCustodyInviteModal
          onDismiss={() => {
            model.dismissSelfCustodyInvite()
          }}
          onConnect={() => {
            model.dismissSelfCustodyInvite()
            connectAccount()
          }}
        />
      )
    }

    return null
  }

  function renderContent() {
    if (loading) {
      return (
        <div className='loading'>
          <ProgressRing />
        </div>
      )
    }

    if (isUnsupportedRegion) {
      return <UnsupportedRegionView />
    }

    if (!paymentId) {
      return (
        <Onboarding
          onOnboardingCompleted={() => setShowOnboardingSuccess(true)}
        />
      )
    }

    if (route === routes.connectAccount) {
      return <ConnectAccount />
    }

    return (
      <>
        <AppFrame>
          <div data-app-route={routes.home}>
            <HomeView />
          </div>
          <div data-app-route={routes.explore}>
            <ExploreView />
          </div>
        </AppFrame>
        {renderModal()}
      </>
    )
  }

  return (
    <div
      className={getClassNames()}
      ref={onMount}
      data-css-scope={style.scope}
    >
      <AppErrorBoundary>{renderContent()}</AppErrorBoundary>
      <div className='background' />
    </div>
  )
}
