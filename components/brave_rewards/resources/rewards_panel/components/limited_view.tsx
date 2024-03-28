/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../shared/lib/locale_context'
import { HostContext, useHostListener } from '../lib/host_context'
import { aboutBATURL } from '../../shared/lib/rewards_urls'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { SettingsIcon } from '../../shared/components/icons/settings_icon'
import { ArrowNextIcon } from '../../shared/components/icons/arrow_next_icon'

import * as style from './limited_view.style'

export function LimitedView () {
  const host = React.useContext(HostContext)
  const { getString, getPluralString } = React.useContext(LocaleContext)

  const [publishersVisitedCount, setPublishersVisitedCount] =
    React.useState(host.state.publishersVisitedCount)
  const [publisherCountText, setPublisherCountText] = React.useState('')

  useHostListener(host, (state) => {
    setPublishersVisitedCount(state.publishersVisitedCount)
  })

  React.useEffect(() => {
    let active = true
    getPluralString('publisherCountText', publishersVisitedCount)
      .then((value) => { active && setPublisherCountText(value) })
    return () => { active = false }
  }, [publishersVisitedCount])

  function onConnectAccount () {
    host.handleExternalWalletAction('verify')
  }

  function onSettingsClick () {
    host.openRewardsSettings()
  }

  function renderConnectBox () {
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
    return (
      <style.publisherSupport>
        <style.publisherCount>
          {publishersVisitedCount}
        </style.publisherCount>
        <div data-test-id='publishers-count'>
          {publisherCountText}
        </div>
      </style.publisherSupport>
    )
  }

  return (
    <style.root>
      <style.header>
        <style.title>
          {getString('headerTitle')}
          <style.headerText>
            {getString('headerText')}
          </style.headerText>
        </style.title>
      </style.header>
      {renderConnectBox()}
      {renderPublisherBox()}
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
