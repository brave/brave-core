/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { withState } from '@dump247/storybook-state'
import { storiesOf } from '@storybook/react'
import { withKnobs, boolean, text } from '@storybook/addon-knobs'

// Components
import { DetailRow as AdsHistoryRow } from '../components/tableAdsHistory'
import { DetailRow as ContributeRow } from '../components/tableContribute'
import { DetailRow as DonationDetailRow } from '../components/tableDonation'
import { DetailRow as PendingDetailRow } from '../components/tablePending'
import {
  ModalContribute,
  ModalBackupRestore,
  ModalActivity,
  ModalDonation,
  ModalPending,
  ModalRedirect,
  ModalShowAdsHistory,
  ModalVerify
} from '../components'

const favicon = require('./img/brave-favicon.png')
const ddgo = require('./img/ddgo.jpg')
const wiki = require('./img/wiki.jpg')
const buzz = require('./img/buzz.jpg')
const guardian = require('./img/guardian.jpg')
const eich = require('./img/eich.jpg')
const tesla = require('./img/tesla.jpg')

const doNothing = () => {
  console.log('nothing')
}

storiesOf('Rewards/Modal', module)
  .addDecorator(withKnobs)
  .add('Backup/Restore', withState({ activeTabId: 0 }, (store) => {
    const onTabChange = () => {
      const newId = store.state.activeTabId === 0 ? 1 : 0
      store.set({ activeTabId: newId })
    }

    return (
      <div style={{ maxWidth: '900px', background: '#fff', padding: '30px' }}>
        <ModalBackupRestore
          funds={'55 BAT'}
          activeTabId={store.state.activeTabId}
          showBackupNotice={boolean('Show backup notice?', false)}
          walletProvider={'Uphold'}
          backupKey={'crouch  hint  glow  recall  round  angry  weasel  luggage save  hood  census  near  still   power  vague  balcony camp  law  now  certain  wagon  affair  butter  choice '}
          error={text('Error', '')}
          onTabChange={onTabChange}
          onClose={doNothing}
          onCopy={doNothing}
          onPrint={doNothing}
          onSaveFile={doNothing}
          onRestore={doNothing}
          onReset={doNothing}
          internalFunds={0}
        />
      </div>
    )
  }))
  .add('Contribute', withState({ activeTabId: 0 }, (store) => {
    const onTabChange = () => {
      const newId = store.state.activeTabId === 0 ? 1 : 0
      store.set({ activeTabId: newId })
    }

    const rows: ContributeRow[] = [
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
      <ModalContribute
        rows={rows}
        excludedRows={rows}
        onTabChange={onTabChange}
        onClose={doNothing}
        onRestore={doNothing}
        activeTabId={store.state.activeTabId}
      />
    )
  }))
  .add('Activity', () => {
    return (
      <ModalActivity
        activityRows={[
          {
            profile: {
              name: 'Bart Baker',
              verified: true,
              provider: 'youtube',
              src: ''
            },
            url: 'https://brave.com',
            amount: {
              tokens: '5.0',
              converted: '5.00'
            },
            type: 'monthly'
          }
        ]}
        transactionRows={[
          {
            date: 1576066103000,
            type: 'ads',
            description: 'Brave Ads payment for May',
            amount: {
              value: '5.0',
              converted: '5.00'
            }
          }
        ]}
        onClose={doNothing}
        onPrint={doNothing}
        onMonthChange={doNothing}
        months={{
          'aug-2019': 'August 2019',
          'jul-2019': 'July 2019',
          'jun-2019': 'June 2019',
          'may-2019': 'May 2019',
          'apr-2019': 'April 2019'
        }}
        summary={[
          {
            type: 'grant',
            token: {
              value: '10.0',
              converted: '5.20'
            }
          },
          {
            type: 'ads',
            token: {
              value: '10.0',
              converted: '5.20'
            }
          },
          {
            type: 'contribute',
            token: {
              value: '10.0',
              converted: '5.20'
            }
          },
          {
            type: 'monthly',
            token: {
              value: '2.0',
              converted: '1.1'
            }
          },
          {
            type: 'tip',
            token: {
              value: '19.0',
              converted: '10.10'
            }
          }
        ]}
      />
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
          tokens: '7.0',
          converted: '3.20'
        },
        text: 'May 2'
      }
    ]
    return (
      <ModalDonation
        rows={rows}
        title={'Tips'}
        onClose={doNothing}
      />
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
      <ModalPending
        rows={rows}
        onClose={doNothing}
        onRemoveAll={doNothing}
      />
    )
  })
  .add('Redirect',() => {
    return (
      <ModalRedirect
        titleText={text('Title text', 'Sorry there was a problem processing your request, please try again.')}
        errorText={'Error explanation, more info here.'}
        onClick={doNothing}
      />
    )
  })
  .add('Verify', () => {
    return (
      <div style={{ width: '373px', minHeight: '580px', position: 'relative', borderRadius: '5px', overflow: 'hidden' }}>
        <ModalVerify
          onVerifyClick={doNothing}
          onClose={doNothing}
          walletProvider={'Uphold'}
        />
      </div>
    )
  })
  .add('Show Ads History',() => {
    const adsPerHour = 2
    const adUuid: number = 0
    const rowId: number = 0
    const rows: AdsHistoryRow[] = [
      {
        uuid: rowId.toString(),
        date: '1/30',
        adDetailRows: [
          {
            uuid: adUuid.toString(),
            adContent: {
              brand: 'Pepsi',
              brandLogo: '',
              brandUrl: 'https://www.pepsi.com',
              brandDisplayUrl: 'pepsi.com',
              brandInfo: 'Animation & VFX Degree - Degree in Animation |',
              adAction: 'view',
              likeAction: 1,
              onThumbUpPress: doNothing,
              onThumbDownPress: doNothing,
              onMenuFlag: doNothing,
              onMenuSave: doNothing,
              savedAd: false,
              flaggedAd: false
            },
            categoryContent: {
              category: 'Entertainment',
              optAction: 0,
              onOptInAction: doNothing,
              onOptOutAction: doNothing
            }
          },
          {
            uuid: (adUuid + 1).toString(),
            adContent: {
              brand: 'TESLA',
              brandLogo: '',
              brandUrl: 'https://www.tesla.com',
              brandDisplayUrl: 'tesla.com',
              brandInfo: 'Animation & VFX Degree - Degree in Animation |',
              adAction: 'click',
              likeAction: 2,
              onThumbUpPress: doNothing,
              onThumbDownPress: doNothing,
              onMenuFlag: doNothing,
              onMenuSave: doNothing,
              savedAd: true,
              flaggedAd: false,
              logoUrl: tesla
            },
            categoryContent: {
              category: 'Technology & Computing',
              optAction: 0,
              onOptInAction: doNothing,
              onOptOutAction: doNothing
            }
          },
          {
            uuid: (adUuid + 2).toString(),
            adContent: {
              brand: 'Disney',
              brandLogo: '',
              brandUrl: 'https://www.disney.com',
              brandDisplayUrl: 'disney.com',
              brandInfo: 'Animation & VFX Degree - Degree in Animation |',
              adAction: 'click',
              likeAction: 0,
              onThumbUpPress: doNothing,
              onThumbDownPress: doNothing,
              onMenuFlag: doNothing,
              onMenuSave: doNothing,
              savedAd: false,
              flaggedAd: false
            },
            categoryContent: {
              category: 'Travel',
              optAction: 0,
              onOptInAction: doNothing,
              onOptOutAction: doNothing
            }
          }
        ]
      },
      {
        uuid: (rowId + 1).toString(),
        date: '1/29',
        adDetailRows: [
          {
            uuid: (adUuid + 3).toString(),
            adContent: {
              brand: 'Puma',
              brandLogo: '',
              brandUrl: 'https://www.puma.com',
              brandDisplayUrl: 'puma.com',
              brandInfo: 'Animation & VFX Degree - Degree in Animation |',
              adAction: 'landed',
              likeAction: 1,
              onThumbUpPress: doNothing,
              onThumbDownPress: doNothing,
              onMenuFlag: doNothing,
              onMenuSave: doNothing,
              savedAd: false,
              flaggedAd: false
            },
            categoryContent: {
              category: 'Sports',
              optAction: 1,
              onOptInAction: doNothing,
              onOptOutAction: doNothing
            }
          },
          {
            uuid: (adUuid + 4).toString(),
            adContent: {
              brand: 'Expedia.com',
              brandLogo: '',
              brandUrl: 'https://www.expedia.com',
              brandDisplayUrl: 'expedia.com',
              brandInfo: 'Animation & VFX Degree - Degree in Animation |',
              adAction: 'view',
              likeAction: 0,
              onThumbUpPress: doNothing,
              onThumbDownPress: doNothing,
              onMenuFlag: doNothing,
              onMenuSave: doNothing,
              savedAd: true,
              flaggedAd: true
            },
            categoryContent: {
              category: 'Travel',
              optAction: 2,
              onOptInAction: doNothing,
              onOptOutAction: doNothing
            }
          },
          {
            uuid: (adUuid + 5).toString(),
            adContent: {
              brand: 'H&M',
              brandLogo: '',
              brandUrl: 'https://www.hm.com',
              brandDisplayUrl: 'hm.com',
              brandInfo: 'Animation & VFX Degree - Degree in Animation |',
              adAction: 'dismiss',
              likeAction: 0,
              onThumbUpPress: doNothing,
              onThumbDownPress: doNothing,
              onMenuFlag: doNothing,
              onMenuSave: doNothing,
              savedAd: true,
              flaggedAd: false
            },
            categoryContent: {
              category: 'Fashion',
              optAction: 1,
              onOptInAction: doNothing,
              onOptOutAction: doNothing
            }
          }
        ]
      }
    ]
    return (
      <ModalShowAdsHistory
        onClose={doNothing}
        rows={rows}
        adsPerHour={adsPerHour}
        hasSavedEntries={true}
        totalDays={7}
      />
    )
  })
  .add('Show Empty Ads History',() => {
    const adsPerHour = 0
    return (
      <ModalShowAdsHistory
        onClose={doNothing}
        rows={undefined}
        adsPerHour={adsPerHour}
      />
    )
  })
