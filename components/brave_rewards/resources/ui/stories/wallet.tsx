/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { storiesOf } from '@storybook/react'
import { withKnobs, object, select, number, text, boolean } from '@storybook/addon-knobs'
// @ts-ignore
import centered from '@storybook/addon-centered/dist'

// Components
import { WalletSummary, WalletEmpty, WalletOff, WalletPanel, WalletSummarySlider, WalletWrapper } from '../../../src/features/rewards'
import { AlertWallet } from '../../../src/features/rewards/walletWrapper'
import { WalletAddIcon, WalletImportIcon } from '../../../src/components/icons'

const bartBaker = require('../../assets/img/bartBaker.jpeg')

const doNothing = () => {
  console.log('nothing')
}

storiesOf('Feature Components/Rewards/Wallet', module)
  .addDecorator(withKnobs)
  .addDecorator(centered)
  .add('Wrapper',() => {
    const alert: AlertWallet = {
      node: 'Some text',
      type: 'success',
      onAlertClose: doNothing
    }
    const showAlert = boolean('Show alert', false)
    const showGrant = boolean('Show grants', false)

    return (
      <WalletWrapper
        compact={false}
        contentPadding={false}
        connectedWallet={boolean('Connected wallet', false)}
        showCopy={boolean('Show Uphold', false)}
        showSecActions={boolean('Show secondary actions', true)}
        tokens={number('Tokens', 25)}
        converted={text('Converted', '163230.50 USD')}
        actions={[
          {
            name: 'Add funds',
            action: doNothing,
            icon: <WalletAddIcon />
          },
          {
            name: 'Withdraw Funds',
            action: doNothing,
            icon: <WalletImportIcon />
          }
        ]}
        grants={showGrant ? [
          {
            tokens: 8,
            expireDate: '7/15/2018'
          },
          {
            tokens: 10,
            expireDate: '9/10/2018'
          },
          {
            tokens: 10,
            expireDate: '10/10/2018'
          }
        ] : []}
        alert={showAlert ? alert : undefined}
      >
       Some content
      </WalletWrapper>
    )
  })
  .add('Empty', () => {
    return (
      <div style={{ width: '373px', background: '#f9fbfc', padding: '0 25px' }}>
        <WalletEmpty/>
      </div>
    )
  })
  .add('Summary', () => {
    return (
      <div style={{ width: '373px', background: '#f9fbfc', padding: '0 25px' }}>
        <WalletSummary
          report={{
            grant: object('Grant', { tokens: 10, converted: 0.25 }),
            deposit: object('Deposit', { tokens: 10, converted: 0.25 }),
            ads: object('Ads', { tokens: 10, converted: 0.25 }),
            contribute: object('Contribute', { tokens: 10, converted: 0.25 }),
            donation: object('Donation', { tokens: 2, converted: 0.25 }),
            tips: object('Tips', { tokens: 19, converted: 5.25 })
          }}
          onActivity={doNothing}
        />
      </div>
    )
  })
  .add('Panel', () => {
    return (
      <div style={{ width: '373px', background: '#f9fbfc' }}>
        <WalletPanel
          id={'wallet-panel'}
          platform={select('Provider', { youtube: 'YouTube', twitter: 'Twitter', twitch: 'Twitch' }, 'youtube')}
          publisherImg={bartBaker}
          publisherName={'Bart Baker'}
          monthlyAmount={number('Amount', 10)}
          isVerified={boolean('Verified', true)}
          tipsEnabled={boolean('Tips Enabled', true)}
          includeInAuto={boolean('Include in monthly', true)}
          attentionScore={'15'}
          donationAmounts={
            [5, 10, 15, 20, 30, 50, 100]
          }
          onToggleTips={doNothing}
          donationAction={doNothing}
          onAmountChange={doNothing}
          onIncludeInAuto={doNothing}
        />
      </div>
    )
  })
  .add('Off', () => {
    return (
      <div style={{ width: '373px', background: '#f9fbfc', padding: '0 25px' }}>
        <WalletOff/>
      </div>
    )
  })
  .add('Summary Slider', () => {
    return (
      <div style={{ width: '373px', padding: '0 25px' }}>
        <WalletSummarySlider
          id={'summary-slider'}
        />
      </div>
    )
  })
