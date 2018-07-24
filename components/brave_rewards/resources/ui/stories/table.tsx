/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { storiesOf, addDecorator } from '@storybook/react'
import { withKnobs, boolean, object, number } from '@storybook/addon-knobs'
import { BetterPageVisualizer } from '../../storyUtil'

// Components
import ContributeTable, { DetailRow as ContributeDetailRow } from '../../../src/features/rewards/contributeTable'
import DonationTable, { DetailRow as DonationDetailRow } from '../../../src/features/rewards/donationTable'

const bart = require('../../assets/img/bartBaker.jpeg')
const ddgo = require('../../assets/img/ddgo.jpg')
const wiki = require('../../assets/img/wiki.jpg')
const buzz = require('../../assets/img/buzz.jpg')
const guardian = require('../../assets/img/guardian.jpg')
const eich = require('../../assets/img/eich.jpg')


addDecorator(withKnobs)

// Globally adapt the story visualizer for this story
addDecorator(BetterPageVisualizer)

storiesOf('Feature Components/Rewards/Table', module)
  .add('Contribution',() => {
      const header: string[] = [
        'Site visited',
        'Attentions'
      ]

      const rows: ContributeDetailRow[] = [
        {
          profile: {
            name: 'Bart Baker',
            verified: true,
            provider: 'youtube',
            src: bart
          },
          attention: 40,
          onRemove: () => {}
        },
        {
          profile: {
            name: 'duckduckgo.com',
            verified: true,
            src: ddgo
          },
          attention: 20,
          onRemove: () => {
          }
        },
        {
          profile: {
            name: 'buzzfeed.com',
            verified: false,
            src: buzz
          },
          attention: 10,
          onRemove: () => {}
        },
        {
          profile: {
            name: 'theguardian.com',
            verified: true,
            src: guardian
          },
          attention: 5,
          onRemove: () => {}
        },
        {
          profile: {
            name: 'wikipedia.org',
            verified: false,
            src: wiki
          },
          attention: 4,
          onRemove: () => {}
        }
      ]
      return <div style={{width: '595px'}}>
        <ContributeTable
          header={object('Header', header)}
          rows={object('Rows', rows)}
          allSites={boolean('Are this all sites?', false)}
          numSites={number('Number of all sites?', 55)}
        >
          Please visit some sites
        </ContributeTable>
      </div>
    })
    .add('Donation',() => {
      const rows: DonationDetailRow[] = [
        {
          profile: {
            name: 'Bart Baker',
            verified: true,
            provider: 'youtube',
            src: bart
          },
          type: 'recurring',
          contribute: {
            tokens: 2,
            converted: 0.2
          },
          onRemove: () => {}
        },
        {
          profile: {
            verified: false,
            name: 'theguardian.com',
            src: guardian
          },
          type: 'donation',
          contribute: {
            tokens: 12,
            converted: 6.2
          },
          text: 'May 7',
        },
        {
          profile: {
            verified: false,
            name: 'BrendanEich',
            provider: 'twitter',
            src: eich
          },
          type: 'tip',
          contribute: {
            tokens: 7,
            converted: 3.2
          },
          text: 'May 2',
        }
      ]
      return <div style={{width: '595px'}}>
        <DonationTable
          rows={object('Rows', rows)}
          allItems={boolean('Are this all items?', false)}
          numItems={number('Number of all items?', 55)}
        >
          Please visit some sites
        </DonationTable>
      </div>
    })
