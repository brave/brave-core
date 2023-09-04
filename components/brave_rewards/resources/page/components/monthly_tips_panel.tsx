/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useActions, useRewardsData } from '../lib/redux_hooks'
import { LocaleContext } from '../../shared/lib/locale_context'

import {
  SettingsPanel,
  PanelHeader,
  PanelTable,
  TokenAmountWithExchange
} from './settings_panel'

import { PageModal } from './page_modal'
import { PublisherLink } from './publisher_link'
import { TrashIcon } from './icons/trash_icon'

import * as style from './monthly_tips_panel.style'

const maxTableSize = 5

const nextContributionFormatter = new Intl.DateTimeFormat(undefined, {
  day: 'numeric',
  month: 'short',
  year: 'numeric'
})

export function MonthlyTipsPanel () {
  const { getString } = React.useContext(LocaleContext)
  const actions = useActions()

  const data = useRewardsData((state) => ({
    parameters: state.parameters,
    reconcileStamp: state.reconcileStamp,
    recurringList: state.recurringList
  }))

  const [showAllModal, setShowAllModal] = React.useState(false)

  const toggleShowAll = () => { setShowAllModal(!showAllModal) }

  function renderNextContribution (time: number) {
    const nextContribution = time * 1000
    if (!nextContribution) {
      return null
    }
    return (
      <>
        {getString('nextContribution')}:&nbsp;
        {nextContributionFormatter.format(new Date(nextContribution))}
      </>
    )
  }

  function renderTable (maxRows?: number) {
    let rows = data.recurringList
    if (maxRows) {
      rows = rows.slice(0, maxRows)
    }

    return (
      <PanelTable>
        <table>
          <thead>
            <tr>
              <th>{getString('site')}</th>
              <th className='number'>{getString('tokens')}</th>
              <th></th>
            </tr>
          </thead>
          <tbody>
            {
              rows.map((item, index) => {
                const removeTip = () => {
                  actions.removeRecurringTip(item.id)
                }
                return (
                  <tr key={index}>
                    <td>
                      <PublisherLink
                        name={item.name}
                        url={item.url}
                        icon={item.favIcon}
                        platform={item.provider}
                        verified={item.status > 0}
                      >
                        {renderNextContribution(item.tipDate || 0)}
                      </PublisherLink>
                    </td>
                    <td className='number'>
                      <TokenAmountWithExchange
                        amount={item.percentage}
                        exchangeRate={data.parameters.rate}
                        exchangeCurrency='USD'
                      />
                    </td>
                    <td className='number'>
                      <style.remove>
                        <button onClick={removeTip}>
                          <TrashIcon />
                        </button>
                      </style.remove>
                    </td>
                  </tr>
                )
              })
            }
          </tbody>
        </table>
        {
          data.recurringList.length === 0 &&
            <div className='empty'>
              {getString('monthlyTipsEmpty')}
            </div>
        }
      </PanelTable>
    )
  }

  return (
    <SettingsPanel deeplinkId='monthly-contributions'>
      <style.root>
        {
          showAllModal &&
            <PageModal
              title={getString('monthlyTipsTitle')}
              onClose={toggleShowAll}
            >
              {renderTable()}
            </PageModal>
        }
        <PanelHeader
          title={getString('monthlyTipsTitle')}
          enabled={true}
          showConfig={false}
        />
        <style.description>
          {getString('monthlyTipsDesc')}
        </style.description>
        {renderTable(maxTableSize)}
        {
          data.recurringList.length > maxTableSize &&
            <style.showAll>
              <button onClick={toggleShowAll}>
                {getString('showAll')}
              </button>
            </style.showAll>
        }
      </style.root>
    </SettingsPanel>
  )
}
