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
import ModalAddFunds, { Address } from '../../../src/features/rewards/modalAddFunds'

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
        url: 'https://brave.com',
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
        url: 'https://brave.com',
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
        url: 'https://brave.com',
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
        url: 'https://brave.com',
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
        url: 'https://brave.com',
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
            text: 'Deposits',
            type: 'deposit',
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
          value: 11,
          converted: 0.5
        }}
        paymentDay={12}
        openBalance={{
          value: 10,
          converted: 5.20
        }}
        closingBalance={{
          value: 21,
          converted: 5.30
        }}
      />
    )
  })
  .add('Add funds', () => {
    const addresses: Address[] = [
      {
        type: 'BTC',
        address: '17fBi3kyqUd2jjPDSi8ArBbMWso16qmxW5',
        qr: 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAALkAAAC5CAAAAABRxsGAAAABz0lEQVR42u3cQY7DIAwF0Nz/0u1uVgP5hqYV8Fh1lFF4qVRjbNrrteq4yMnJycnJycnJycnJyXeRX/ejfcO/q6X/q89GTn62PJiz+Wdz9v/uXJqNnJy8/blvRo9mLGg+V302cnLyAXm6jAfBiZyc/EF5kJ+n9yMnJ5/Pz4OZSnvu7+0syMmXlwdr+Qdffbk6R06+sjxu1YRpeD9f+EWHi5x8ZXnwuW8+QwAs1a3JycnjwJEm34OtpFJuT05+tjyIMkHtqm6bjy3k5MfI09JT0A5KL8THqcjJT5WnnddHwkqAJicnj/fDaa82zRzmO7nk5MfI+4SgxBz0eftv22hsISffVx6Ei9J0/TJYvQBNTk6ebJSDq6Os6bMW5OT7ypO1NzwhVdpf99MMcnLyuPP68HONrv7k5GfJSy2d+nY7jUHk5OQ38mDOWokqXP3nYws5+W7ydPRjUD3vLu25ycmPlV/3I60zB6cp+q/mO1zk5FvK04hSP9FU+qrAJ6tz5OS7yeuHK9IE4aoMcnLyUflgzCgVvsnJyedjS1oQSytq89U5cvLN5fWraWoerPTk5OTzv4FWf8z0uR6szpGTryxfYpCTk5OTk5OTk5OTk5OvON4QJEO8FpFK4QAAAABJRU5ErkJggg=='
      },
      {
        type: 'ETH',
        address: '0xF10bfc0EB8Fcfd1240a5BB97C3e5a7752cD1C388',
        qr: 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAM0AAADNCAAAAAA+16u1AAACPklEQVR42u3bwU7DMBAE0P7/T8ONA1LcGXsTFfRyKpQ2+3IY2bvm9fWfrhcNDQ0NDQ0NDQ0NDQ0NDc2p5vX+6m7y84lfry6/ebMCGhqaSU1QW/CJX2+kT6KvgIaG5h7NZd5UubQu6/KbgwpoaGg+QNNn2iWYhobm72rSzwZxRkND81GatGex+bG14eHdGg0NTd/rvPnVw51bGhqajTHj8iZplWlL5O4pLg0NTbC7qIruextBqtLQ0DyiqQaYl8LL363Dbv04hlY2NDQ0qSYoK4ikILqC5c15ptHQ0FSa4E792HJzhHo08aChoTmeEfQ3Duae/RLqqMtBQ0NzsrIJGhebtfWtz/P9DQ0NTZVpgStY43T9yrBDQkND84imGmX2rYn10qiKMxoamnFNuhxJrZvnH9MGBw0NzbgmbWakZaVReHIaioaGZkhTRVe1sgkmKNXGh4aG5h7NzHQybWb0OXfU66ShoakmHsGwI9gMBXk4s8ahoaEZ0lSdy+CU08k/TQ0lNA0NzaYmaFqmG5X+EY13bmloaI4zbXOP0g871o9tKKFpaGjSTAuOL6Y90X7Pk8YZDQ3NpCZuKoQHodYxlbY5aWhontO83l/rP+6LmTlQTUNDM6lJkyzY7lSLpODJ0tDQPKfZLDCIs2p4Mtm5paGhuVmzLjrY5KQNTxoamo/XHGfaxgiGhoZmWpO+GxxcSmNq8+ADDQ3NuCbdbAQ/phuV4I9v7NzS0ND8g4uGhoaGhoaGhoaGhoaGhqa5vgFTleQ0sHcoKgAAAABJRU5ErkJggg=='
      },
      {
        type: 'BAT',
        address: '0xF10bfc0EB8Fcfd1240a5BB97C3e5a7752cD1C388',
        qr: 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAM0AAADNCAAAAAA+16u1AAACPklEQVR42u3bwU7DMBAE0P7/T8ONA1LcGXsTFfRyKpQ2+3IY2bvm9fWfrhcNDQ0NDQ0NDQ0NDQ0NDc2p5vX+6m7y84lfry6/ebMCGhqaSU1QW/CJX2+kT6KvgIaG5h7NZd5UubQu6/KbgwpoaGg+QNNn2iWYhobm72rSzwZxRkND81GatGex+bG14eHdGg0NTd/rvPnVw51bGhqajTHj8iZplWlL5O4pLg0NTbC7qIruextBqtLQ0DyiqQaYl8LL363Dbv04hlY2NDQ0qSYoK4ikILqC5c15ptHQ0FSa4E792HJzhHo08aChoTmeEfQ3Duae/RLqqMtBQ0NzsrIJGhebtfWtz/P9DQ0NTZVpgStY43T9yrBDQkND84imGmX2rYn10qiKMxoamnFNuhxJrZvnH9MGBw0NzbgmbWakZaVReHIaioaGZkhTRVe1sgkmKNXGh4aG5h7NzHQybWb0OXfU66ShoakmHsGwI9gMBXk4s8ahoaEZ0lSdy+CU08k/TQ0lNA0NzaYmaFqmG5X+EY13bmloaI4zbXOP0g871o9tKKFpaGjSTAuOL6Y90X7Pk8YZDQ3NpCZuKoQHodYxlbY5aWhontO83l/rP+6LmTlQTUNDM6lJkyzY7lSLpODJ0tDQPKfZLDCIs2p4Mtm5paGhuVmzLjrY5KQNTxoamo/XHGfaxgiGhoZmWpO+GxxcSmNq8+ADDQ3NuCbdbAQ/phuV4I9v7NzS0ND8g4uGhoaGhoaGhoaGhoaGhqa5vgFTleQ0sHcoKgAAAABJRU5ErkJggg=='
      },
      {
        type: 'LTC',
        address: 'Le8aswhmGJjn9jP5teEWdyJARak4xU8sCn',
        qr: 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAM0AAADNCAAAAAA+16u1AAACOUlEQVR42u3bQXLDIAwF0Nz/0u0FYvKFBNN0Hit7mtq8LDTwRV4//2m8aGhoaGhoaGhoaGhoaGhouprX57H+8LvnvbstPXT9DhoamnHNc+F4fNZyWmvX0AxoaGimNe/qyONf16UrqILBO2hoaL5Is4YESx4aGpov0pRK13prQ0ND86c0gbX0zvX0g/nS0NDc0wRJ4+Gry8ktDQ3NRptxL7ksTfBeF5eGhibYXQSR5mPtS/dBHTANDc2kJkguSwUrtbYaojQ0NEc06e1mSVq7gmCUhobmoCbIHIPOZsnVWd7Q0NCMa9Iwo9TZ3MgwwwyEhoZmXBN0MR+vgmVL50QEDQ3NPU2w4Ci1LUvdzsmUg4aGpt73TE8lBWlIqYhNdjxoaGg6fc/A1d6ZpKcjk7fR0NAMatJ5pM3PYPsU/Ec/s6GhoUlXNmlforRsqR9yCLqiNDQ045pSWekccCr1VvtZJw0NTSfrDEpXUBnTk9HpXouGhua0Zv3AzW7n+iMH+540NDT1rHN97LkejsRLlM9nrmloaO5pSpFmWr+Cn1qkSy0aGprLmtJpqHVcEViH9jc0NDQlTTo2s41SejF5oouGhqaTdaYlKbgKdk6lM9I0NDRnNEElS0OP9DtJ26o0NDT3NEG96fQvgv3Smd0aDQ3NoGZzPVPqc9DQ0PxlTTC3/orlc8BBQ0NzRpMGmeltUMTW3ywNDc09TZBPBH3PemFrlVEaGpoZzdcOGhoaGhoaGhoaGhoaGhqayvgFbnvHJxkVZlQAAAAASUVORK5CYII='
      }
    ]

    return (
      <ModalAddFunds
        addresses={addresses}
        onClose={doNothing}
      />
    )
  })
