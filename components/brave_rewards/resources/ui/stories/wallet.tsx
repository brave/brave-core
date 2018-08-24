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
import { Type } from '../../../src/features/rewards/alert'
import { WalletAddIcon, WalletImportIcon } from '../../../src/components/icons'

const doNothing = () => {
  console.log('nothing')
}

storiesOf('Feature Components/Rewards/Wallet', module)
  .addDecorator(withKnobs)
  .addDecorator(centered)
  .add('Wrapper',() => {
    const alert = {
      node: 'Some text',
      type: 'success' as Type,
      onAlertClose: doNothing
    }
    const showAlert = boolean('Show alert', false)

    return (
      <WalletWrapper
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
          grant={object('Grant', { tokens: 10, converted: 0.25 })}
          deposit={object('Deposit', { tokens: 10, converted: 0.25 })}
          ads={object('Ads', { tokens: 10, converted: 0.25 })}
          contribute={object('Contribute', { tokens: 10, converted: 0.25 })}
          donation={object('Donation', { tokens: 2, converted: 0.25 })}
          tips={object('Tips', { tokens: 19, converted: 5.25 })}
          total={object('Total', { tokens: 1, converted: 5.25 })}
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
