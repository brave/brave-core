/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { storiesOf } from '@storybook/react'
import { withKnobs, object, number, text, boolean } from '@storybook/addon-knobs'
// @ts-ignore
import centered from '@storybook/addon-centered/dist'

// Components
import { WalletSummary, WalletEmpty, WalletOff, WalletWrapper } from '../../../src/features/rewards'

// Assets
const wallet = require('../../assets/img/rewards_wallet.svg')
const funds = require('../../assets/img/rewards_funds.svg')

const doNothing = () => {
  console.log('nothing')
}

storiesOf('Feature Components/Rewards/Wallet', module)
  .addDecorator(withKnobs)
  .addDecorator(centered)
  .add('Base',() => {
    return (
      <WalletWrapper
        tokens={number('Tokens', 25)}
        converted={text('Converted', '163230.50 USD')}
        actions={[
          {
            name: 'Add funds',
            action: doNothing,
            icon: wallet
          },
          {
            name: 'Withdraw Funds',
            action: doNothing,
            icon: funds
          }
        ]}
        showCopy={boolean('Show Uphold', false)}
        showSecActions={boolean('Show secondary actions', true)}
        connectedWallet={boolean('Connected wallet', false)}
        grants={object('Grants', [
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
        ])}
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
          grant={object('Grant', { color: '#C12D7C', tokens: 10, converted: 0.25 })}
          ads={object('Ads', { color: '#C12D7C', tokens: 10, converted: 0.25 })}
          contribute={object('Contribute', { color: '#9752CB', tokens: 10, converted: 0.25 })}
          donation={object('Donation', { color: '#4C54D2', tokens: 2, converted: 0.25 })}
          tips={object('Tips', { color: '#4C54D2', tokens: 19, converted: 5.25 })}
          onActivity={doNothing}
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
