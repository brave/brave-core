/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { storiesOf, addDecorator } from '@storybook/react'
import { withKnobs, object, number, text, boolean } from '@storybook/addon-knobs'
import { BetterPageVisualizer } from '../../storyUtil'

// Components
import { PanelSummary, PanelEmpty, PanelOff, Panel } from '../../../src/features/rewards';

// Assets
const wallet = require('../../assets/img/rewards_wallet.svg')
const funds = require('../../assets/img/rewards_funds.svg')

addDecorator(withKnobs)

// Globally adapt the story visualizer for this story
addDecorator(BetterPageVisualizer)

storiesOf('Feature Components/Rewards/Wallet', module)
  .add('Base',() => {
    return <Panel
      tokens={number('Tokens', 25)}
      converted={text('Converted', '163230.50 USD')}
      actions={[
        {
          name: 'Add funds',
          action: () => {},
          icon: wallet
        },
        {
          name: 'Withdraw Funds',
          action: () => {},
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
    </Panel>
  })
  .add('Empty', () => {
    return <div style={{width: '373px', background: '#f9fbfc', padding: '0 25px'}}><PanelEmpty/></div>
  })
  .add('Summary', () => {
    return <div style={{width: '373px', background: '#f9fbfc', padding: '0 25px'}}>
      <PanelSummary
        grant={object('Grant', {color: '#C12D7C', tokens: 10, converted: 0.25})}
        ads={object('Ads', {color: '#C12D7C', tokens: 10, converted: 0.25})}
        contribute={object('Contribute', {color: '#9752CB', tokens: 10, converted: 0.25})}
        donation={object('Donation', {color: '#4C54D2', tokens: 2, converted: 0.25})}
        tips={object('Tips', {color: '#4C54D2', tokens: 19, converted: 5.25})}
        grants={[
          {
            id: '1',
            tokens: 15,
            converted: 0.75
          },
          {
            id: '2',
            tokens: 10,
            converted: 0.50
          }
        ]}
        onActivity={()=>{}}
      />
    </div>
  })
  .add('Off', () => {
    return <div style={{width: '373px', background: '#f9fbfc', padding: '0 25px'}}><PanelOff/></div>
  })
