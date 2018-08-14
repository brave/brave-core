/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { withState } from '@dump247/storybook-state'
import { storiesOf } from '@storybook/react'
import { withKnobs, text } from '@storybook/addon-knobs'

// Components
import { TabsType } from '../../../src/features/rewards/modalBackupRestore'
import { DetailRow as ContributeRow } from '../../../src/features/rewards/tableContribute'
import { DetailRow as TransactionsRow } from '../../../src/features/rewards/tableTransactions'
import { ModalContribute, ModalBackupRestore } from '../../../src/features/rewards'
import ModalActivity from '../../../src/features/rewards/modalActivity'

const bart = require('../../assets/img/bartBaker.jpeg')
const ddgo = require('../../assets/img/ddgo.jpg')
const wiki = require('../../assets/img/wiki.jpg')
const buzz = require('../../assets/img/buzz.jpg')
const guardian = require('../../assets/img/guardian.jpg')

const doNothing = () => {
  console.log('nothing')
}

storiesOf('Feature Components/Rewards/Modal', module)
  .addDecorator(withKnobs)
  .add('Backup/Restore', withState({ tabId: 'backup' }, (store) => {
    const onTabChange = (tabId: string) => {
      store.set({ tabId })
    }

    return (
      <div style={{ maxWidth: '900px', background: '#fff', padding: '30px' }}>
        <ModalBackupRestore
          activeTabId={store.state.tabId as TabsType}
          backupKey={'crouch  hint  glow  recall  round  angry  weasel  luggage save  hood  census  near  still   power  vague  balcony camp  law  now  certain  wagon  affair  butter  choice '}
          error={text('Error', '')}
          onTabChange={onTabChange}
          onClose={doNothing}
          onCopy={doNothing}
          onPrint={doNothing}
          onSaveFile={doNothing}
          onRestore={doNothing}
        />
      </div>
    )
  }))
  .add('Contribute', () => {
    const rows: ContributeRow[] = [
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
      <ModalContribute
        rows={rows}
        onClose={doNothing}
      />
    )
  })
  .add('Activity', () => {
    const contributions: ContributeRow[] = [
      {
        profile: {
          name: 'Bart Baker',
          verified: true,
          provider: 'youtube',
          src: bart
        },
        attention: 40,
        onRemove: doNothing,
        token: {
          value: 5,
          converted: 5
        }
      },
      {
        profile: {
          name: 'duckduckgo.com',
          verified: true,
          src: ddgo
        },
        attention: 20,
        onRemove: doNothing,
        token: {
          value: 4,
          converted: 11
        }
      },
      {
        profile: {
          name: 'buzzfeed.com',
          verified: false,
          src: buzz
        },
        attention: 10,
        onRemove: doNothing,
        token: {
          value: 3,
          converted: 15
        }
      },
      {
        profile: {
          name: 'theguardian.com',
          verified: true,
          src: guardian
        },
        attention: 5,
        onRemove: doNothing,
        token: {
          value: 2,
          converted: 17
        }
      },
      {
        profile: {
          name: 'wikipedia.org',
          verified: false,
          src: wiki
        },
        attention: 4,
        onRemove: doNothing,
        token: {
          value: 1,
          converted: 11
        }
      }
    ]

    const transactions: TransactionsRow[] = [
      {
        date: '6/1',
        type: 'deposit',
        description: 'Brave Ads payment for May',
        amount: {
          value: 5,
          converted: 5
        }
      },
      {
        date: '6/9',
        type: 'tipOnLike',
        description: {
          publisher: 'Bart Baker',
          platform: 'YouTube'
        },
        amount: {
          isNegative: true,
          value: 5,
          converted: 11
        }
      },
      {
        date: '6/10',
        type: 'deposit',
        description: 'Token grant made available or unlocked',
        amount: {
          value: 10,
          converted: 15
        }
      },
      {
        date: '6/12',
        type: 'donation',
        description: 'coinmarketcap.com',
        amount: {
          isNegative: true,
          value: 10,
          converted: 15
        }
      },
      {
        date: '6/14',
        type: 'tipOnLike',
        description: {
          publisher: 'BrendanEich',
          platform: 'Twitter'
        },
        amount: {
          isNegative: true,
          value: 1,
          converted: 2
        }
      },
      {
        date: '6/26',
        type: 'deposit',
        description: 'Added via Uphold',
        amount: {
          value: 10,
          converted: 15
        }
      },
      {
        date: '6/31',
        type: 'contribute',
        description: 'Monthly payment',
        amount: {
          isNegative: true,
          value: 10,
          converted: 15
        }
      },
      {
        date: '6/31',
        type: 'recurringDonation',
        description: 'Monthly payment',
        amount: {
          isNegative: true,
          value: 5,
          converted: 15
        }
      }
    ]

    return (
      <ModalActivity
        contributeRows={contributions}
        transactionRows={transactions}
        onClose={doNothing}
        onPrint={doNothing}
        onDownloadPDF={doNothing}
        onMonthChange={doNothing}
        months={{ 'jun-2018': 'June 2018', 'may-2018': 'May 2018', 'apr-2018': 'April 2018' }}
        currentMonth={'jun-2018'}
        summary={[
          {
            text: 'Token Grant available',
            type: 'grant',
            token: {
              value: 10,
              converted: 5.20
            }
          },
          {
            text: 'Earnings from Brave Ads',
            type: 'ads',
            token: {
              value: 10,
              converted: 5.20
            }
          },
          {
            text: 'Brave Contribute',
            type: 'contribute',
            notPaid: true,
            token: {
              value: 10,
              converted: 5.20,
              isNegative: true
            }
          },
          {
            text: 'Recurring Donations',
            type: 'recurring',
            notPaid: true,
            token: {
              value: 2,
              converted: 1.1,
              isNegative: true
            }
          },
          {
            text: 'One-time Donations/Tips',
            type: 'donations',
            token: {
              value: 19,
              converted: 10.10,
              isNegative: true
            }
          }
        ]}
        total={{
          value: 1,
          converted: 0.5
        }}
        paymentDay={12}
        openBalance={{
          value: 10,
          converted: 5.20
        }}
        closingBalance={{
          value: 11,
          converted: 5.30
        }}
      />
    )
  })
