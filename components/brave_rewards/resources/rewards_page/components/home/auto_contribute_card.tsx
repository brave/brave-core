/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import Toggle from '@brave/leo/react/toggle'

import { AutoContributeEntry } from '../../lib/app_model'
import { useLocaleContext } from '../../lib/locale_strings'
import { formatMessage } from '../../../shared/lib/locale_context'
import { AppModelContext, useAppState } from '../../lib/app_model_context'
import { NewTabLink } from '../../../shared/components/new_tab_link'
import { getCreatorIconSrc, getCreatorPlatformIcon } from '../../lib/creator_icon'

import { style } from './auto_contribute_card.style'

const nextContributionFormatter = new Intl.DateTimeFormat(undefined, {
  month: 'long',
  day: 'numeric'
})

const amountFormatter = new Intl.NumberFormat(undefined, {
  minimumFractionDigits: 2
})

const attentionFormatter = new Intl.NumberFormat(undefined, {
  style: 'percent'
})

export function AutoContributeCard() {
  const { getString } = useLocaleContext()
  const model = React.useContext(AppModelContext)

  const [externalWallet, acInfo, parameters] = useAppState((state) => [
    state.externalWallet,
    state.autoContributeInfo,
    state.rewardsParameters
  ])

  const [showInfoText, setShowInfoText] = React.useState(false)
  const [showAll, setShowAll] = React.useState(false)

  function renderEntry(entry: AutoContributeEntry) {
    const { site, attention } = entry
    return (
      <div key={entry.site.id} className='site'>
        <span className='icon'>
          <NewTabLink href={site.url}>
            <img src={getCreatorIconSrc(site)} alt='Site icon' />
          </NewTabLink>
        </span>
        <span className='name'>
          <NewTabLink href={site.url}>
            {site.name}
            {site.platform &&
              <Icon name={getCreatorPlatformIcon(site)} forceColor />}
          </NewTabLink>
        </span>
        <span className='attention'>
          {attentionFormatter.format(attention)}
        </span>
        <button onClick={() => model.removeAutoContributeSite(site.id)}>
          <Icon name='trash' />
        </button>
      </div>
    )
  }

  function renderSiteList() {
    if (!acInfo) {
      return null
    }

    if (acInfo.entries.length === 0) {
      return (
        <p className='empty'>
          {getString('acEmptyListText')}
        </p>
      )
    }

    const topEntries = [...acInfo.entries]
      .sort((a, b) => b.attention - a.attention)
      .slice(0, showAll ? Infinity : 5)

    return (
      <div className='site-list'>
        <div className='heading'>
          <span>{getString('acSiteLabel')}</span>
          <span>{getString('acAttentionLabel')}</span>
        </div>
        {topEntries.map(renderEntry)}
        {
          acInfo.entries.length > topEntries.length &&
            <div className='show-all'>
              <Button kind='plain' onClick={() => setShowAll(true)}>
                {getString('showAllButtonLabel')}
              </Button>
            </div>
        }
      </div>
    )
  }

  function renderEnabled() {
    if (!acInfo || !parameters) {
      return null
    }

    let { amount } = acInfo
    if (!amount) {
      amount = parameters.autoContributeChoice
    }

    const amounts = [...parameters.autoContributeChoices]
    if (!amounts.includes(amount)) {
      amounts.push(amount)
    }
    amounts.sort((a, b) => a - b)

    return (
      <section>
        <div className='info'>
          <button
            className='title'
            onClick={() => setShowInfoText(!showInfoText)}
          >
            <Icon name='info-outline' />
            <span className='text'>{getString('acInfoTitle')}</span>
            <Icon name={showInfoText ? 'carat-up' : 'carat-down'} />
          </button>
          {showInfoText && <p>{getString('acInfoText')}</p>}
        </div>
        <div className='settings'>
          <div>
            <label>{getString('acAmountLabel')}</label>
            <span className='value'>
              <select
                className='subtle'
                value={amount}
                onChange={(event) => {
                  model.setAutoContributeAmount(
                    parseFloat(event.currentTarget.value))
                }}
              >
                {
                  amounts.map((amount) => (
                    <option key={amount} value={amount}>
                      {
                        formatMessage(getString('acAmountText'), [
                          amountFormatter.format(amount)
                        ])
                      }
                    </option>
                  ))
                }
              </select>
            </span>
          </div>
          <div>
            <label>{getString('acSiteCountLabel')}</label>
            <span className='value'>{acInfo.entries.length}</span>
          </div>
          <div>
            <label>{getString('acNextContributionLabel')}</label>
            <span className='value'>
              {
                nextContributionFormatter.format(
                  new Date(acInfo.nextAutoContributeDate))
              }
            </span>
          </div>
        </div>
        {renderSiteList()}
      </section>
    )
  }

  function renderDisabled() {
    return (
      <section>
        <div className='disabled'>
          <Icon name='hand-coins' />
          <div className='title'>
            {getString('acDisabledTitle')}
          </div>
          <div className='text'>
            {getString('acDisabledText')}
          </div>
        </div>
      </section>
    )
  }

  if (!externalWallet || !acInfo) {
    return null
  }

  return (
    <div className='content-card' {...style}>
      <h4>
        <span>{getString('acTitle')}</span>
        <Toggle
          checked={acInfo.enabled}
          onChange={(detail) => {
            model.setAutoContributeEnabled(detail.checked)
          }}
        />
      </h4>
      {acInfo.enabled ? renderEnabled() : renderDisabled()}
    </div>
  )
}
