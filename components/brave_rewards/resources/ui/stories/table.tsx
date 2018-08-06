/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { storiesOf } from '@storybook/react'
import { withKnobs, boolean, object, number } from '@storybook/addon-knobs'
// @ts-ignore
import centered from '@storybook/addon-centered/dist'

// Components
import TableContribute, { DetailRow as ContributeDetailRow } from '../../../src/features/rewards/tableContribute'
import TableDonation, { DetailRow as DonationDetailRow } from '../../../src/features/rewards/tableDonation'

const bart = require('../../assets/img/bartBaker.jpeg')
const ddgo = require('../../assets/img/ddgo.jpg')
const wiki = require('../../assets/img/wiki.jpg')
const buzz = require('../../assets/img/buzz.jpg')
const guardian = require('../../assets/img/guardian.jpg')
const eich = require('../../assets/img/eich.jpg')

const doNothing = () => {
  console.log('nothing')
}

storiesOf('Feature Components/Rewards/Table', module)
  .addDecorator(withKnobs)
  .addDecorator(centered)
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
        onRemove: doNothing
      },
      {
        profile: {
          name: 'duckduckgo.com',
          verified: true,
          src: ddgo
        },
        attention: 20,
        onRemove: doNothing
      },
      {
        profile: {
          name: 'buzzfeed.com',
          verified: false,
          src: buzz
        },
        attention: 10,
        onRemove: doNothing
      },
      {
        profile: {
          name: 'theguardian.com',
          verified: true,
          src: guardian
        },
        attention: 5,
        onRemove: doNothing
      },
      {
        profile: {
          name: 'wikipedia.org',
          verified: false,
          src: wiki
        },
        attention: 4,
        onRemove: doNothing
      }
    ]
    return (
      <div style={{ width: '595px' }}>
        <TableContribute
          header={object('Header', header)}
          rows={object('Rows', rows)}
          allSites={boolean('Are this all sites?', false)}
          numSites={number('Number of all sites?', 55)}
          showRowAmount={boolean('Show row amount', false)}
          showRemove={boolean('Show remove action', true)}
        >
          Please visit some sites
        </TableContribute>
      </div>
    )
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
        onRemove: doNothing
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
        text: 'May 7'
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
        text: 'May 2'
      }
    ]
    return (
      <div style={{ width: '595px' }}>
        <TableDonation
          rows={object('Rows', rows)}
          allItems={boolean('Are this all items?', false)}
          numItems={number('Number of all items?', 55)}
        >
          Please visit some sites
        </TableDonation>
      </div>
    )
  })
