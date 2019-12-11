/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { storiesOf } from '@storybook/react'
import { withKnobs, boolean, object, number } from '@storybook/addon-knobs'
import centered from '@storybook/addon-centered'

// Components
import TableContribute, { DetailRow as ContributeDetailRow } from '../components/tableContribute'
import TableDonation, { DetailRow as DonationDetailRow } from '../components/tableDonation'
import TableTransactions, { DetailRow as TransactionsRow } from '../components/tableTransactions'
import TablePending, { DetailRow as PendingDetailRow } from '../components/tablePending'

const favicon = require('./img/brave-favicon.png')
const ddgo = require('./img/ddgo.jpg')
const wiki = require('./img/wiki.jpg')
const buzz = require('./img/buzz.jpg')
const guardian = require('./img/guardian.jpg')
const eich = require('./img/eich.jpg')

const doNothing = () => {
  console.log('nothing')
}

storiesOf('Rewards/Table', module)
  .addDecorator(withKnobs)
  .addDecorator(centered)
  .add('Contribution', () => {
    const header: string[] = [
      'Site',
      'Attention'
    ]

    const rows: ContributeDetailRow[] = [
      {
        profile: {
          name: 'Jonathon Doe',
          verified: true,
          provider: 'youtube',
          src: favicon
        },
        url: 'https://brave.com',
        attention: 40,
        onRemove: doNothing
      },
      {
        profile: {
          name: 'duckduckgo.com',
          verified: true,
          src: ddgo
        },
        url: 'https://brave.com',
        attention: 20,
        onRemove: doNothing
      },
      {
        profile: {
          name: 'buzzfeed.com',
          verified: false,
          src: buzz
        },
        url: 'https://brave.com',
        attention: 10,
        onRemove: doNothing
      },
      {
        profile: {
          name: 'theguardian.com',
          verified: true,
          src: guardian
        },
        url: 'https://brave.com',
        attention: 5,
        onRemove: doNothing
      },
      {
        profile: {
          name: 'wikipedia.org',
          verified: false,
          src: wiki
        },
        url: 'https://brave.com',
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
          headerColor={boolean('Colored header', true)}
        >
          Please visit some sites
        </TableContribute>
      </div>
    )
  })
  .add('Donation', () => {
    const rows: DonationDetailRow[] = [
      {
        profile: {
          name: 'Jonathon Doe',
          verified: true,
          provider: 'youtube',
          src: favicon
        },
        url: 'https://brave.com',
        type: 'recurring',
        contribute: {
          tokens: '2.0',
          converted: '0.20'
        },
        onRemove: doNothing
      },
      {
        profile: {
          verified: false,
          name: 'theguardian.com',
          src: guardian
        },
        url: 'https://brave.com',
        type: 'donation',
        contribute: {
          tokens: '12000.0',
          converted: '6000.20'
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
        url: 'https://brave.com',
        type: 'tip',
        contribute: {
          tokens: '1.0',
          converted: '0.20'
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
          headerColor={boolean('Colored header', true)}
        >
          Please visit some sites
        </TableDonation>
      </div>
    )
  })
  .add('Transactions', () => {
    const rows: TransactionsRow[] = [
      {
        date: 1576066103000,
        type: 'ads',
        description: 'Brave Ads payment for May',
        amount: {
          value: '5.0',
          converted: '5.00'
        }
      },
      {
        date: 1576066103000,
        type: 'tip',
        description: {
          publisher: 'Jonathon Doe',
          platform: 'YouTube'
        },
        amount: {
          isNegative: true,
          value: '5.0',
          converted: '11.00'
        }
      },
      {
        date: 1576066103000,
        type: 'contribute',
        description: 'Monthly payment',
        amount: {
          isNegative: true,
          value: '5.0',
          converted: '15.00'
        }
      }
    ]
    return (
      <div style={{ width: '595px' }}>
        <TableTransactions
          rows={object('Rows', rows)}
        >
          Sorry no transactions.
        </TableTransactions>
      </div>
    )
  })
  .add('Pending contributions',() => {
    const rows: PendingDetailRow[] = [
      {
        profile: {
          name: 'Jonathon Doe',
          verified: true,
          provider: 'youtube',
          src: favicon
        },
        url: 'https://brave.com',
        type: 'recurring',
        amount: {
          tokens: '2.0',
          converted: '0.20'
        },
        date: 'Jan 2',
        onRemove: doNothing
      },
      {
        profile: {
          verified: false,
          name: 'theguardian.com',
          src: guardian
        },
        url: 'https://brave.com',
        type: 'tip',
        amount: {
          tokens: '12000.0',
          converted: '6000.20'
        },
        date: 'May 7',
        onRemove: doNothing
      },
      {
        profile: {
          verified: false,
          name: 'BrendanEich',
          provider: 'twitter',
          src: eich
        },
        url: 'https://brave.com',
        type: 'ac',
        amount: {
          tokens: '1.0',
          converted: '0.20'
        },
        date: 'May 2',
        onRemove: doNothing
      }
    ]
    return (
      <div style={{ width: '595px' }}>
        <TablePending
          rows={object('Rows', rows)}
        >
          Please visit some sites
        </TablePending>
      </div>
    )
  })
