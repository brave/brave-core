/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { HostContext, useHostListener } from '../lib/host_context'
import { WalletCard } from '../../shared/components/wallet_card'
import { NavBar } from './navbar'
import { PanelOverlays } from './panel_overlays'
import { PublisherCard } from './publisher_card'

type ActiveView = 'tip' | 'summary'

export function Panel () {
  const host = React.useContext(HostContext)

  const [balance, setBalance] = React.useState(host.state.balance)
  const [settings, setSettings] = React.useState(host.state.settings)
  const [externalWallet, setExternalWallet] =
    React.useState(host.state.externalWallet)
  const [exchangeInfo, setExchangeInfo] =
    React.useState(host.state.exchangeInfo)
  const [earningsInfo, setEarningsInfo] =
    React.useState(host.state.earningsInfo)
  const [summaryData, setSummaryData] = React.useState(host.state.summaryData)
  const [publisherInfo, setPublisherInfo] =
    React.useState(host.state.publisherInfo)

  const [activeView, setActiveView] = React.useState<ActiveView>(
    publisherInfo ? 'tip' : 'summary')

  useHostListener(host, (state) => {
    setBalance(state.balance)
    setSettings(state.settings)
    setExternalWallet(state.externalWallet)
    setExchangeInfo(state.exchangeInfo)
    setEarningsInfo(state.earningsInfo)
    setSummaryData(state.summaryData)
    setPublisherInfo(state.publisherInfo)
  })

  return (
    <div>
      <div className='rewards-panel' data-test-id='rewards-panel'>
        <WalletCard
          balance={balance}
          externalWallet={externalWallet}
          earningsThisMonth={earningsInfo.earningsThisMonth}
          earningsLastMonth={earningsInfo.earningsLastMonth}
          nextPaymentDate={earningsInfo.nextPaymentDate}
          exchangeRate={exchangeInfo.rate}
          exchangeCurrency={exchangeInfo.currency}
          showSummary={activeView === 'summary'}
          summaryData={summaryData}
          autoContributeEnabled={settings.autoContributeEnabled}
          onExternalWalletAction={host.handleExternalWalletAction}
        />
        {activeView === 'tip' && <PublisherCard />}
        <NavBar
          canTip={Boolean(publisherInfo)}
          activeView={activeView}
          onActiveViewChange={setActiveView}
          onSettingsClick={host.openRewardsSettings}
        />
      </div>
      <PanelOverlays />
    </div>
  )
}
