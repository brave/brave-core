// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { object, text, select, boolean, number } from '@storybook/addon-knobs'

// Components
import { WalletSummary, WalletEmpty, WalletPanel, WalletSummarySlider, WalletWrapper } from '../components'
import { AlertWallet, WalletState } from '../components/walletWrapper'
import { WalletAddIcon, WalletImportIcon } from 'brave-ui/components/icons'
import { WalletInfoHeader } from '../components/mobile'

const favicon = require('./img/brave-favicon.png')

const doNothing = () => {
  console.log('nothing')
}

export default {
  title: 'Rewards/Wallet/Desktop',
  parameters: {
    layout: 'centered'
  }
}

export const wrapper = () => {
  const onEnableAds = () => {
    console.log('enabling ads')
  }
  const alert: AlertWallet = {
    node: 'Some text',
    type: 'success',
    onAlertClose: doNothing
  }
  const showAlert = boolean('Show alert', false)

  const state: WalletState = select('wallet status', {
    unverified: 'unverified',
    verified: 'verified',
    'disconnected unverified': 'disconnected_unverified',
    'disconnected verified': 'disconnected_verified'
  }, 'unverified') as WalletState

  return (
    <WalletWrapper
      compact={false}
      contentPadding={false}
      onNotificationClick={onEnableAds}
      notification={undefined}
      showCopy={boolean('Show Uphold', false)}
      showSecActions={boolean('Show secondary actions', true)}
      balance={text('Balance', '25.0')}
      converted={text('Converted', '163230.50 USD')}
      actions={[
        {
          name: 'Add funds',
          action: doNothing,
          icon: <WalletAddIcon />,
          externalWallet: true
        },
        {
          name: 'Withdraw Funds',
          action: doNothing,
          icon: <WalletImportIcon />,
          externalWallet: true
        }
      ]}
      alert={showAlert ? alert : undefined}
      walletState={state}
      walletProvider={'Uphold'}
      onDisconnectClick={doNothing}
      greetings={'Hello, Brave!'}
    >
      Some content
    </WalletWrapper>
  )
}

export const empty = () => {
  return (
    <div style={{ width: '373px', background: '#f9fbfc', padding: '0 25px' }}>
      <WalletEmpty />
    </div>
  )
}

export const summary = () => {
  return (
    <div style={{ width: '373px', background: '#f9fbfc', padding: '0 25px' }}>
      <WalletSummary
        report={{
          grant: object('Grant', { tokens: '10.0', converted: '0.25' }),
          deposit: object('Deposit', { tokens: '10.0', converted: '0.25' }),
          ads: object('Ads', { tokens: '10.0', converted: '0.25' }),
          contribute: object('Contribute', { tokens: '10.0', converted: '0.25' }),
          donation: object('Donation', { tokens: '2.0', converted: '0.25' }),
          tips: object('Tips', { tokens: '19.0', converted: '5.25' })
        }}
        onActivity={doNothing}
        reservedAmount={number('Reserved amount', 52)}
        reservedMoreLink={'https://brave.com'}
        onSeeAllReserved={doNothing}
      />
    </div>
  )
}

export const panel = () => {
  return (
    <div style={{ width: '373px', background: '#fff' }}>
      <WalletPanel
        id={'wallet-panel'}
        platform={select<any>('Provider', { youtube: 'youtube', twitter: 'twitter', twitch: 'twitch', reddit: 'reddit', vimeo: 'vimeo', github: 'github' }, 'youtube')}
        publisherImg={favicon}
        publisherName={'Jonathan Doe'}
        monthlyAmount={'10.000'}
        isVerified={boolean('Verified', true)}
        tipsEnabled={boolean('Tips Enabled', true)}
        includeInAuto={boolean('Include in monthly', true)}
        acEnabled={boolean('AC enabled?', true)}
        attentionScore={'15'}
        onToggleTips={doNothing}
        setMonthlyAction={doNothing}
        cancelMonthlyAction={doNothing}
        donationAction={doNothing}
        onIncludeInAuto={doNothing}
        showUnVerified={boolean('Show unverified content', true)}
        onRefreshPublisher={doNothing}
      />
    </div>
  )
}

export const summarySlider = () => {
  return (
    <div style={{ width: '373px', padding: '0 25px' }}>
      <WalletSummarySlider
        id={'summary-slider'}
      />
    </div>
  )
}

export const walletInfoHeaderMobile = () => {
  return (
    <div style={{ position: 'fixed', top: 0, left: 0, height: '100vh', width: '100%' }}>
      <div style={{ width: '100%', position: 'absolute', top: '30%' }}>
        <WalletInfoHeader
          id={'info-header'}
          onClick={doNothing}
          balance={text('Balance', '30.0')}
          converted={text('Converted', '163230.50 USD')}
        />
      </div>
    </div>
  )
}
