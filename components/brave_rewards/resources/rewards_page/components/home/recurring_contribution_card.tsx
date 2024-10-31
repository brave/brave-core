/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import Icon from '@brave/leo/react/icon'

import { RecurringContribution } from '../../lib/app_state'
import { AppModelContext, useAppState } from '../../lib/app_model_context'
import { useLocaleContext } from '../../lib/locale_strings'
import { getCreatorIconSrc, getCreatorPlatformIcon } from '../../lib/creator_icon'
import { NewTabLink } from '../../../shared/components/new_tab_link'

import { style } from './recurring_contribution_card.style'

const amountFormatter = new Intl.NumberFormat(undefined, {
  minimumFractionDigits: 2
})

const exchangeFormatter = new Intl.NumberFormat(undefined, {
  style: 'currency',
  currency: 'USD'
})

const nextContributionFormatter = new Intl.DateTimeFormat(undefined)

export function RecurringContributionCard() {
  const { getString } = useLocaleContext()
  const model = React.useContext(AppModelContext)

  const [externalWallet, contributions, parameters] = useAppState((state) => [
    state.externalWallet,
    state.recurringContributions,
    state.rewardsParameters
  ])

  const [showAll, setShowAll] = React.useState(false)

  function renderItem(contribution: RecurringContribution) {
    const { site, amount, nextContributionDate } = contribution

    if (!parameters) {
      return null
    }

    return (
      <div key={contribution.site.id}>
        <div className='icon'>
          <NewTabLink href={site.url}>
            <img src={getCreatorIconSrc(site)} alt='Site icon' />
          </NewTabLink>
        </div>
        <div className='name'>
          <div>
            <NewTabLink href={site.url}>
              {site.name}
              {site.platform &&
                <Icon name={getCreatorPlatformIcon(site)} forceColor />}
            </NewTabLink>
          </div>
          <div className='next-contribution'>
            <Icon name='calendar-check' />
            {getString('recurringNextContributionLabel')}{' '}
            {nextContributionFormatter.format(new Date(nextContributionDate))}
          </div>
        </div>
        <div className='amount'>
          <div>{amountFormatter.format(amount)} BAT</div>
          <div className='exchange-amount'>
            {exchangeFormatter.format(amount * parameters.rate)}
          </div>
        </div>
        <div className='more'>
          <ButtonMenu>
            <button slot='anchor-content'>
              <Icon name='more-vertical' />
            </button>
            <leo-menu-item
              onClick={() => model.removeRecurringContribution(site.id)}
            >
              <Icon name='trash' />
              {getString('removeButtonLabel')}
            </leo-menu-item>
          </ButtonMenu>
        </div>
      </div>
    )
  }

  function renderList() {
    if (contributions.length === 0) {
      return <p className='empty'>{getString('recurringListEmptyText')}</p>
    }

    const topEntries = [...contributions]
      .sort((a, b) => a.nextContributionDate - b.nextContributionDate)
      .slice(0, showAll ? Infinity : 5)

    return <>
      <div className='list'>
        {topEntries.map(renderItem)}
      </div>
      {
        contributions.length > topEntries.length &&
          <div className='show-all'>
            <Button kind='plain' onClick={() => setShowAll(true)}>
              {getString('showAllButtonLabel')}
            </Button>
          </div>
      }
    </>
  }

  if (!externalWallet) {
    return null
  }

  return (
    <div className='content-card' {...style}>
      <h4>{getString('recurringTitle')}</h4>
      <section>
        {renderList()}
      </section>
    </div>
  )
}
