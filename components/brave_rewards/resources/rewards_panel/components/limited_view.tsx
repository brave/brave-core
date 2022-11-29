/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../shared/lib/locale_context'
import { HostContext, useHostListener } from '../lib/host_context'
import { supportedWalletRegionsURL, aboutBATURL } from '../../shared/lib/rewards_urls'
import { ToggleButton } from '../../shared/components/toggle_button'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { SettingsIcon } from '../../shared/components/icons/settings_icon'
import { ArrowNextIcon } from '../../shared/components/icons/arrow_next_icon'
import { FancyBatIcon } from './icons/fancy_bat_icon'

import * as derivedState from '../lib/derived_state'
import * as style from './limited_view.style'

export function LimitedView () {
  const host = React.useContext(HostContext)
  const { getString, getPluralString } = React.useContext(LocaleContext)

  const [adsEnabled, setAdsEnabled] =
    React.useState(host.state.settings.adsEnabled)
  const [publishersVisitedCount, setPublishersVisitedCount] =
    React.useState(host.state.publishersVisitedCount)
  const [publisherCountText, setPublisherCountText] = React.useState('')
  const [canConnectAccount, setCanConnectAccount] =
    React.useState(derivedState.canConnectAccount(host.state))

  useHostListener(host, (state) => {
    setAdsEnabled(state.settings.adsEnabled)
    setPublishersVisitedCount(state.publishersVisitedCount)
    setCanConnectAccount(derivedState.canConnectAccount(state))
  })

  React.useEffect(() => {
    let active = true
    getPluralString('publisherCountText', publishersVisitedCount)
      .then((value) => { active && setPublisherCountText(value) })
    return () => { active = false }
  }, [publishersVisitedCount])

  function onToggleAdsEnabled () {
    host.setAdsEnabled(!adsEnabled)
  }

  function onConnectAccount () {
    host.handleExternalWalletAction('verify')
  }

  function onSettingsClick () {
    host.openRewardsSettings()
  }

  function renderConnectBox () {
    if (!canConnectAccount) {
      return (
        <style.connect>
          <div>
            {getString('connectAccountNoProviders')}
          </div>
          <style.connectLearnMore>
            <NewTabLink href={supportedWalletRegionsURL}>
              {getString('learnMore')}
            </NewTabLink>
          </style.connectLearnMore>
        </style.connect>
      )
    }

    return (
      <style.connect>
        <style.connectAction>
          <button onClick={onConnectAccount}>
            {getString('rewardsConnectAccount')}<ArrowNextIcon />
          </button>
        </style.connectAction>
        <div>
          {
            formatMessage(getString('connectAccountText'), {
              tags: {
                $1: (content) => <strong key='bold'>{content}</strong>
              }
            })
          }
        </div>
      </style.connect>
    )
  }

  function renderPublisherBox () {
    if (publishersVisitedCount < 1) {
      return null
    }

    return (
      <style.publisherSupport>
        <style.publisherCount>
          {publishersVisitedCount}
        </style.publisherCount>
        <div>
          {publisherCountText}
        </div>
      </style.publisherSupport>
    )
  }

  function renderContent () {
    if (!adsEnabled) {
      return (
        <style.disabledText>
          <div><FancyBatIcon /></div>
          <div>{getString('aboutRewardsText')}</div>
        </style.disabledText>
      )
    }

    return (
      <>
        {renderConnectBox()}
        {renderPublisherBox()}
      </>
    )
  }

  return (
    <style.root>
      <style.header>
        <style.title>
          {getString('headerTitle')}
          <style.headerText>
            {
              adsEnabled
                ? getString('headerTextAdsEnabled')
                : getString('headerTextAdsDisabled')
            }
          </style.headerText>
        </style.title>
        <style.rewardsSwitch>
          <ToggleButton
            checked={adsEnabled}
            onChange={onToggleAdsEnabled}
          />
        </style.rewardsSwitch>
      </style.header>
      {renderContent()}
      <style.settings>
        <button onClick={onSettingsClick}>
          <SettingsIcon /> {getString('rewardsSettings')}
        </button>
      </style.settings>
      <style.learnMore>
        <NewTabLink href={aboutBATURL}>
          {getString('learnMoreAboutBAT')}
        </NewTabLink>
      </style.learnMore>
    </style.root>
  )
}
