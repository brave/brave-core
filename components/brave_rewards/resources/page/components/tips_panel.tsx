/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useActions, useRewardsData } from '../lib/redux_hooks'
import { PlatformContext } from '../lib/platform_context'
import { LocaleContext } from '../../shared/lib/locale_context'
import { lookupPublisherPlatformName } from '../../shared/lib/publisher_platform'

import {
  SettingsPanel,
  PanelHeader,
  PanelItem,
  PanelTable,
  TokenAmountWithExchange,
  ConfigHeader
} from './settings_panel'

import { ToggleButton } from '../../shared/components/toggle_button'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { PageModal } from './page_modal'
import { PublisherLink } from './publisher_link'

import * as urls from '../../shared/lib/rewards_urls'
import * as style from './tips_panel.style'

const maxTableSize = 5

export function TipsPanel () {
  const { isAndroid } = React.useContext(PlatformContext)
  const { getString } = React.useContext(LocaleContext)
  const actions = useActions()

  const data = useRewardsData((state) => ({
    parameters: state.parameters,
    tipsList: state.tipsList,
    inlineTip: state.inlineTip
  }))

  const [showAllModal, setShowAllModal] = React.useState(false)
  const [showConfig, setShowConfig] = React.useState(false)
  const [needsRestart, setNeedsRestart] = React.useState(false)

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
              rows.map((item) => {
                return (
                  <tr key={item.id}>
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

  function renderConfig () {
    const toggleHandler = (site: string) => {
      return (enabled: boolean) => {
        actions.onInlineTipSettingChange(site, enabled)
        setNeedsRestart(true)
      }
    }

    const onRelaunch = () => { actions.restartBrowser() }

    return (
      <>
        <ConfigHeader />
        <style.description>
          {getString('donationAbility')}
        </style.description>
        {
          Object.keys(data.inlineTip).map((site) => (
            <PanelItem key={site} label={lookupPublisherPlatformName(site)}>
              <ToggleButton
                checked={data.inlineTip[site]}
                onChange={toggleHandler(site)}
              />
            </PanelItem>
          ))
        }
        {
          needsRestart &&
            <style.restart>
              <button onClick={onRelaunch}>
                {getString('relaunch')}
              </button>
            </style.restart>
        }
      </>
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
    <SettingsPanel>
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
          showConfig={showConfig}
          onShowConfigChange={isAndroid ? undefined : setShowConfig}
        />
        {showConfig ? renderConfig() : renderContent()}
      </style.root>
    </SettingsPanel>
  )
}
