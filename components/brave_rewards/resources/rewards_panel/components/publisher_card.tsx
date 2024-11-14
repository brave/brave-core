/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import Icon from '@brave/leo/react/icon'

import { LocaleContext, formatMessage } from '../../shared/lib/locale_context'
import { getPublisherPlatformName } from '../../shared/lib/publisher_platform'
import { HostContext, useHostListener } from '../lib/host_context'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { TokenAmount } from '../../shared/components/token_amount'
import { LoadingIcon } from '../../shared/components/icons/loading_icon'
import { InfoIcon } from './icons/info_icon'
import { RefreshStatusIcon } from './icons/refresh_status_icon'

import * as style from './publisher_card.style'

const unverifiedLearnMoreURL = 'https://brave.com/faq/#unclaimed-funds'

export function PublisherCard () {
  const { getString } = React.useContext(LocaleContext)
  const host = React.useContext(HostContext)

  const [publisherInfo, setPublisherInfo] =
    React.useState(host.state.publisherInfo)
  const [publisherRefreshing, setPublisherRefreshing] =
    React.useState(host.state.publisherRefreshing)

  const [showPublisherLoading, setShowPublisherLoading] = React.useState(false)

  useHostListener(host, (state) => {
    setPublisherInfo(state.publisherInfo)
    setPublisherRefreshing(state.publisherRefreshing)
  })

  if (!publisherInfo) {
    return null
  }

  const publisherVerified = publisherInfo.verified

  function renderStatusIndicator () {
    if (!publisherInfo) {
      return null
    }

    return (
      <style.statusIndicator>
        <style.verifiedIcon>
          {
            publisherVerified ?
              <Icon name='verification-filled-color' /> :
              <Icon name='verification-filled' />
          }
        </style.verifiedIcon>
        <div>
          {
            getString(
              publisherVerified ? 'verifiedCreator' : 'unverifiedCreator')
          }
        </div>
      </style.statusIndicator>
    )
  }

  function onRefreshClick () {
    // Show the publisher loading state for a minimum amount of time in order
    // to indicate activity to the user.
    setShowPublisherLoading(true)
    setTimeout(() => { setShowPublisherLoading(false) }, 500)

    host.refreshPublisherStatus()
  }

  function onMonthlyTipClick () {
    host.openRewardsSettings()
  }

  function getPublisherName () {
    if (!publisherInfo) {
      return null
    }

    if (publisherInfo.platform) {
      return formatMessage(getString('platformPublisherTitle'), [
        publisherInfo.name,
        getPublisherPlatformName(publisherInfo.platform)
      ])
    }

    return publisherInfo.name
  }

  function renderContributionInfo () {
    if (!publisherInfo) {
      return null
    }

    if (!publisherVerified) {
      return (
        <style.unverifiedNote>
          <div>
            <InfoIcon />
          </div>
          <div>
            {getString('unverifiedText')}
            <div>
              <NewTabLink href={unverifiedLearnMoreURL}>
                {getString('learnMore')}
              </NewTabLink>
            </div>
          </div>
        </style.unverifiedNote>
      )
    }

    return (
      <>
        <style.contribution>
          {
            publisherInfo.monthlyTip > 0 &&
              <style.monthlyTip>
                <div>{getString('monthlyTip')}</div>
                <style.monthlyTipAmount>
                  <button
                    onClick={onMonthlyTipClick}
                    data-test-id='monthly-tip-button'
                  >
                    <TokenAmount
                      amount={publisherInfo.monthlyTip}
                      minimumFractionDigits={0}
                    />
                  </button>
                </style.monthlyTipAmount>
              </style.monthlyTip>
          }
        </style.contribution>
      </>
    )
  }

  return (
    <style.root data-test-id='publisher-card'>
      <style.heading>
        <style.icon>
          <img
            className={publisherInfo.platform ? 'rounded' : ''}
            src={publisherInfo.icon}
          />
        </style.icon>
        <style.name>
          {getPublisherName()}
          <style.status>
            {renderStatusIndicator()}
            <style.refreshStatus>
              {
                publisherRefreshing || showPublisherLoading
                  ? <LoadingIcon />
                  : <button
                      data-test-id='refresh-publisher-button'
                      onClick={onRefreshClick}
                      title={getString('refreshStatus')}
                    >
                      <RefreshStatusIcon />
                    </button>
              }
            </style.refreshStatus>
          </style.status>
        </style.name>
      </style.heading>
      {renderContributionInfo()}
      <style.tipAction>
        <button
          data-test-id='tip-button'
          onClick={host.sendTip}
          disabled={!publisherVerified}
        >
          {getString('sendTip')}
        </button>
      </style.tipAction>
    </style.root>
  )
}
