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
import { Onboarding } from './onboarding/onboarding'
import { OnboardingSuccessModal } from './onboarding/onboarding_success_modal'
import { ConnectAccount } from './connect_account'
import { AuthorizationModal } from './authorization_modal'
import { CaptchaModal } from './captcha_modal'
import { ContributeModal } from './contribute/contribute_modal'
import { ResetModal } from './reset_modal'
import { TosUpdateModal } from './tos_update_modal'
import { NotificationModal } from './notification_modal'
import { SelfCustodyInviteModal } from './self_custody_invite_modal'
import { useShouldShowSelfCustodyInvite } from '../lib/self_custody_invite'
import { useConnectAccountRouter } from '../lib/connect_account_router'
import * as routes from '../lib/app_routes'

import { style } from './app.style'

export function App() {
  const model = React.useContext(AppModelContext)
  const eventHub = React.useContext(EventHubContext)

  const [
    loading,
    embedder,
    paymentId,
    tosUpdateRequired,
    captchaInfo,
    notifications,
  ] = useAppState((state) => [
    state.loading,
    state.embedder,
    state.paymentId,
    state.tosUpdateRequired,
    state.captchaInfo,
    state.notifications
  ])

  const viewType = useBreakpoint()

  const [showResetModal, setShowResetModal] = React.useState(false)
  const [showContributeModal, setShowContributeModal] = React.useState(false)
  const [hideCaptcha, setHideCaptcha] = React.useState(false)
  const [showOnboardingSuccess, setShowOnboardingSuccess]
    = React.useState(false)

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

  function onMount(elem: HTMLElement | null) {
    if (elem) {
      elem.style.setProperty(
        '--app-screen-height',
        window.screen.availHeight + 'px')
    }
  }

  function getClassNames() {
    const list: string[] = []
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

  function renderMainView() {
    switch (route) {
      case routes.creators:
        return <></>
      case routes.explore:
        return <></>
      default:
        return <HomeView />
    }
  }

  function renderContent() {
    if (loading) {
      return (
        <div className='loading'>
          <ProgressRing />
        </div>
      )
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
          {renderMainView()}
        </AppFrame>
        {renderModal()}
      </>
    )
  }

  return (
    <div className={getClassNames()} ref={onMount} {...style}>
      <AppErrorBoundary>
        {renderContent()}
      </AppErrorBoundary>
      <div className='background' />
    </div>
  )
}
