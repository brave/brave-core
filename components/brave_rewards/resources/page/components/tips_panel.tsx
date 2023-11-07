/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useRewardsData } from '../lib/redux_hooks'
import { LocaleContext } from '../../shared/lib/locale_context'

import {
  SettingsPanel,
  PanelHeader,
  PanelItem,
  PanelTable,
  TokenAmountWithExchange
} from './settings_panel'

import { NewTabLink } from '../../shared/components/new_tab_link'
import { PageModal } from './page_modal'
import { PublisherLink } from './publisher_link'

import * as urls from '../../shared/lib/rewards_urls'
import * as style from './tips_panel.style'

const maxTableSize = 5

export function TipsPanel () {
  const { getString } = React.useContext(LocaleContext)

  const data = useRewardsData((state) => ({
    parameters: state.parameters,
    tipsList: state.tipsList
  }))

  const [showAllModal, setShowAllModal] = React.useState(false)

  const totalTips = data.tipsList.reduce(
    (total, item) => total + item.percentage, 0)

  const toggleShowAll = () => { setShowAllModal(!showAllModal) }

  function renderTable (maxRows?: number) {
    let rows = data.tipsList
    if (maxRows) {
      rows = rows.slice(0, maxRows)
    }

    return (
      <PanelTable>
        <table>
          <thead>
            <tr>
              <th>{getString('site')}</th>
              <th>{getString('date')}</th>
              <th className='number'>{getString('tokens')}</th>
            </tr>
          </thead>
          <tbody>
            {
              rows.map((item, index) => {
                return (
                  <tr key={index}>
                    <td>
                      <PublisherLink
                        name={item.name}
                        url={item.url}
                        icon={item.favIcon}
                        platform={item.provider}
                        verified={item.status > 0}
                      />
                    </td>
                    <td>
                      {
                        item.tipDate &&
                          new Date(item.tipDate * 1000).toLocaleDateString()
                      }
                    </td>
                    <td className='number'>
                      <TokenAmountWithExchange
                        amount={item.percentage}
                        exchangeRate={data.parameters.rate}
                        exchangeCurrency='USD'
                      />
                    </td>
                  </tr>
                )
              })
            }
          </tbody>
        </table>
        {
          data.tipsList.length === 0 &&
            <div className='empty'>
              {getString('donationVisitSome')}
            </div>
        }
      </PanelTable>
    )
  }

  function renderContent () {
    return (
      <>
        <style.description>
          {getString('donationDesc')}&nbsp;
          <NewTabLink href={urls.tippingLearnMoreURL}>
            {getString('donationDescLearnMore')}
          </NewTabLink>
        </style.description>
        <PanelItem label={getString('donationTotalDonations')}>
          <span data-test-id='tip-total'>
            <TokenAmountWithExchange
              amount={totalTips}
              exchangeRate={data.parameters.rate}
              exchangeCurrency='USD'
            />
          </span>
        </PanelItem>
        {renderTable(maxTableSize)}
        {
          data.tipsList.length > maxTableSize &&
            <style.showAll>
              <button onClick={toggleShowAll}>
                {getString('showAll')}
              </button>
            </style.showAll>
        }
      </>
    )
  }

  return (
    <SettingsPanel deeplinkId='contributions'>
      <style.root>
        {
          showAllModal &&
            <PageModal
              title={getString('donationTitle')}
              onClose={toggleShowAll}
            >
              {renderTable()}
            </PageModal>
        }
        <PanelHeader
          title={getString('donationTitle')}
          enabled={true}
          showConfig={false}
          onShowConfigChange={undefined}
        />
        {renderContent()}
      </style.root>
    </SettingsPanel>
  )
}
