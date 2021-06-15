/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { withState } from '@dump247/storybook-state'
import { storiesOf } from '@storybook/react'
import { withKnobs, boolean, text, object, select } from '@storybook/addon-knobs'

// Components
import Settings from './settings/settings'
import SettingsMobile from './settingsMobile/settingsMobile'
import {
  DisabledPanel,
  WalletPanel,
  WalletSummary,
  WalletSummarySlider,
  WalletWrapper
} from '../components'
import { BatColorIcon, WalletAddIcon } from 'brave-ui/components/icons'
import { Notification, WalletState } from '../components/walletWrapper'

const favicon = require('./img/brave-favicon.png')

const captchaDrop = require('./img/captchaDrop.png')

const doNothing = (id: string) => {
  console.log('nothing')
}

const defaultGrant = {
  amount: 2.5,
  expiresAt: 1574451334789,
  type: 1,
  status: 0,
  promotionId: 'test',
  hint: '',
  finishTitle: 'It\'s your lucky day!',
  finishText: 'Your token grant is on its way.',
  finishTokenTitle: 'Free Token Grant'
}

const grantNotification = {
  id: '001',
  type: 'grant',
  date: 'July 7',
  onCloseNotification: doNothing,
  text: <span>Free 30 BAT have been awarded to you.</span>
}

const dummyOptInAction = () => {
  console.log(dummyOptInAction)
}

storiesOf('Rewards/Concepts/Desktop', module)
  .addDecorator(withKnobs)
  .add('Settings Page', () => {
    const walletState: WalletState = select('wallet status', {
      unverified: 'unverified',
      verified: 'verified',
      'disconnected unverified': 'disconnected_unverified',
      'disconnected verified': 'disconnected_verified'
    }, 'unverified') as WalletState

    type walletContent = 'empty' | 'summary' | 'off'
    const content: walletContent = select('Content', {
      empty: 'empty',
      summary: 'summary',
      off: 'off'
    }, 'empty') as any

    const walletProps = {
      grants: object('Claimed grants', [
        {
          amount: 2.5,
          expiresAt: '1574451334789',
          type: 1
        },
        {
          amount: 5.0,
          expiresAt: '1574451334789',
          type: 1
        },
        {
          amount: 7.5,
          expiresAt: '1574451334789',
          type: 1
        }
      ]),
      content,
      walletState
    }
    return (<Settings {...{ walletProps }}/>)
  })
  .add('Disabled Panel', () => {
    const onPrivateLink = () => {
      console.log('open up private tab info')
    }

    const doNothing = () => {
      console.log('nothing')
    }

    return (
      <div style={{ position: 'absolute', top: '40px', left: '40px', borderRadius: '8px', overflow: 'hidden' }}>
        <WalletWrapper
          compact={true}
          contentPadding={false}
          gradientTop={'249,251,252'}
          balance={text('Tokens', '30.0')}
          converted={text('Converted', '15.50 USD')}
          walletProvider={'Uphold'}
          actions={[
            {
              name: 'Add funds',
              action: doNothing,
              icon: <WalletAddIcon />,
              externalWallet: true
            },
            {
              name: 'Settings',
              action: doNothing,
              icon: <BatColorIcon />,
              externalWallet: false
            }
          ]}
          showSecActions={false}
        />
        <div style={{ position: 'absolute', top: '40px', left: '500px', width: '330px', borderRadius: '8px', overflow: 'hidden' }}>
          <DisabledPanel isPrivate={true} onLinkOpen={onPrivateLink} />
        </div>
      </div>
    )
  })
  .add('Wallet Panel', withState({ grant: defaultGrant, notification: grantNotification, showSummary: false, tipsEnabled: true, includeInAuto: true, refreshingPublisher: false, publisherRefreshed: false, verified: false }, (store) => {
    const curveRgb = '233,235,255'
    const panelRgb = '249,251,252'

    const getGradientColor = () => {
      return store.state.showSummary ? curveRgb : panelRgb
    }

    const doNothing = () => {
      console.log('do nothing')
    }

    const onCloseNotification = () => {
      console.log(onCloseNotification)
    }

    const onSummaryToggle = () => {
      store.set({ showSummary: !store.state.showSummary })
    }

    const onIncludeInAuto = () => {
      store.set({ includeInAuto: !store.state.includeInAuto })
    }

    const onRefreshPublisher = () => {
      store.set({ refreshingPublisher: !store.state.refreshingPublisher })
      setTimeout(() => {
        store.set({
          refreshingPublisher: !store.state.refreshingPublisher,
          publisherRefreshed: !store.state.publisherRefreshed,
          verified: true
        })
      }, 3000)
    }

    const onToggleTips = () => {
      store.set({ tipsEnabled: !store.state.tipsEnabled })
    }

    const onFetchCaptcha = () => {
      const hint = 'blue'
      const captcha = captchaDrop
      const newGrant = {
        ...store.state.grant,
        captcha,
        hint
      }
      store.set({ grant: newGrant })
    }

    const onGrantHide = () => {
      const hint = ''
      const captcha = ''
      const newGrant = {
        ...store.state.grant,
        captcha,
        hint
      }
      store.set({ grant: newGrant })
    }

    const onSolution = (promotionId: string, x: number, y: number) => {
      const expiryTime = 99
      const newGrant = {
        ...store.state.grant,
        expiryTime
      }
      store.set({ grant: newGrant })
    }

    const onFinish = () => {
      store.set({ grant: undefined })
      store.set({ notification: undefined })
    }

    const onVerifyClick = () => console.log('onVerifyClick')
    const onDisconnectClick = () => console.log('onDisconnectClick')

    return (
      <div style={{ position: 'relative' }}>

        <WalletWrapper
          compact={true}
          contentPadding={false}
          notification={boolean('show notification', true) ?
                        store.state.notification as Notification : undefined}
          gradientTop={getGradientColor()}
          balance={text('Tokens', '30.0')}
          converted={text('Converted', '15.50 USD')}
          actions={[
            {
              name: 'Add funds',
              action: doNothing,
              icon: <WalletAddIcon />,
              externalWallet: true
            },
            {
              name: 'Settings',
              action: doNothing,
              icon: <BatColorIcon />,
              externalWallet: false
            }
          ]}
          showCopy={boolean('Show Uphold', false)}
          showSecActions={false}
          grant={store.state.grant}
          onGrantHide={onGrantHide}
          onNotificationClick={onFetchCaptcha}
          onSolution={onSolution}
          onFinish={onFinish}
          walletState={select('wallet status', {
            unverified: 'unverified',
            verified: 'verified',
            'disconnected unverified': 'disconnected_unverified',
            'disconnected verified': 'disconnected_verified'
          }, 'unverified') as WalletState}
          walletProvider={'Uphold'}
          onVerifyClick={onVerifyClick}
          onDisconnectClick={onDisconnectClick}
          greetings={text('Greetings', 'Hello, Brave!')}
        >
          <WalletSummarySlider
            id={'panel-slider'}
            onToggle={onSummaryToggle}
          >
            <WalletPanel
              id={'wallet-panel'}
              platform={'youtube'}
              publisherImg={favicon}
              publisherName={'Jonathon Doe'}
              monthlyAmount={text('Monthly contribution', '5.0')}
              acEnabled={true}
              isVerified={boolean('Verified', false)}
              showUnVerified={boolean('Show Unverified', true)}
              tipsEnabled={boolean('Tips enabled', store.state.tipsEnabled)}
              includeInAuto={boolean('Tips enabled', store.state.includeInAuto)}
              attentionScore={'17'}
              onToggleTips={onToggleTips}
              donationAction={doNothing}
              onIncludeInAuto={onIncludeInAuto}
              refreshingPublisher={store.state.refreshingPublisher}
              publisherRefreshed={store.state.publisherRefreshed}
              onRefreshPublisher={onRefreshPublisher}
              setMonthlyAction={doNothing}
              cancelMonthlyAction={doNothing}
            />
            <WalletSummary
              compact={true}
              report={{
                grant: object('Grant', { tokens: '10.0', converted: '0.25' }),
                ads: object('Ads', { tokens: '10.0', converted: '0.25' }),
                contribute: object('Contribute', { tokens: '10.0', converted: '0.25' }),
                donation: object('Donation', { tokens: '2.0', converted: '0.25' }),
                tips: object('Tips', { tokens: '19.0', converted: '5.25' })
              }}
            />
          </WalletSummarySlider>
        </WalletWrapper>
      </div>
    )
  }))
storiesOf('Rewards/Concepts/Mobile', module)
  .add('Settings', () => <SettingsMobile />)
