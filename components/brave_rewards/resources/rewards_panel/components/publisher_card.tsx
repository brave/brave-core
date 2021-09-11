/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../shared/lib/locale_context'
import { getPublisherPlatformName } from '../../shared/lib/publisher_platform'
import { HostContext, useHostListener } from '../lib/host_context'
import { MonthlyTipAction } from '../lib/interfaces'
import { ToggleButton } from './toggle_button'
import { MonthlyTipView } from './monthly_tip_view'
import { VerifiedIcon } from './icons/verified_icon'
import { LoadingIcon } from './icons/loading_icon'

import * as style from './publisher_card.style'

export function PublisherCard () {
  const { getString } = React.useContext(LocaleContext)
  const host = React.useContext(HostContext)

  const [publisherInfo, setPublisherInfo] =
    React.useState(host.state.publisherInfo)
  const [publisherRefreshing, setPublisherRefreshing] =
    React.useState(host.state.publisherRefreshing)
  const [settings, setSettings] = React.useState(host.state.settings)

  const [showPublisherLoading, setShowPublisherLoading] = React.useState(false)

  useHostListener(host, (state) => {
    setPublisherInfo(state.publisherInfo)
    setPublisherRefreshing(state.publisherRefreshing)
    setSettings(host.state.settings)
  })

  if (!publisherInfo) {
    return null
  }

  function renderStatusMessage () {
    if (!publisherInfo) {
      return null
    }

    if (publisherInfo.registered) {
      return (
        <style.verified>
          <VerifiedIcon />{getString('verifiedCreator')}
        </style.verified>
      )
    }

    return (
      <style.unverified>
        <VerifiedIcon />{getString('unverifiedCreator')}
      </style.unverified>
    )
  }

  function onRefreshClick () {
    // Show the publisher loading state for a minimum amount of time in order
    // to indicate activity to the user.
    setShowPublisherLoading(true)
    setTimeout(() => { setShowPublisherLoading(false) }, 500)

    host.refreshPublisherStatus()
  }

  function monthlyTipHandler (action: MonthlyTipAction) {
    return () => {
      host.handleMonthlyTipAction(action)
    }
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

  return (
    <style.root>
      <style.heading>
        {
          publisherInfo.icon &&
            <style.icon>
              <img src={publisherInfo.icon} />
            </style.icon>
        }
        <style.name>
          {getPublisherName()}
          <style.status>
            {renderStatusMessage()}
            <style.refreshStatus>
              {
                publisherRefreshing || showPublisherLoading
                  ? <LoadingIcon />
                  : <button onClick={onRefreshClick}>
                      {getString('refreshStatus')}
                    </button>
              }
            </style.refreshStatus>
          </style.status>
        </style.name>
      </style.heading>
      {
        settings.autoContributeEnabled &&
          <style.attention data-test-id='attention-score-text'>
            <div>{getString('attention')}</div>
            <div className='value'>
              {(publisherInfo.attentionScore * 100).toFixed(0)}%
            </div>
          </style.attention>
      }
      <style.contribution>
        {
          settings.autoContributeEnabled &&
            <style.autoContribution>
              <div>{getString('includeInAutoContribute')}</div>
              <div>
                <ToggleButton
                  checked={publisherInfo.autoContributeEnabled}
                  onChange={host.setIncludeInAutoContribute}
                />
              </div>
            </style.autoContribution>
        }
        <style.monthlyContribution>
          <div>{getString('monthlyContribution')}</div>
          <div>
            <MonthlyTipView
              publisherInfo={publisherInfo}
              onUpdateClick={monthlyTipHandler('update')}
              onCancelClick={monthlyTipHandler('cancel')}
            />
          </div>
        </style.monthlyContribution>
      </style.contribution>
      <style.tipAction>
        <button data-test-id='tip-button' onClick={host.sendTip}>
          {getString('sendTip')}
        </button>
      </style.tipAction>
    </style.root>
  )
}
