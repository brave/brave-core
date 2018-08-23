/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { withState } from '@dump247/storybook-state'
import { storiesOf } from '@storybook/react'
import { withKnobs, boolean, text, object, number } from '@storybook/addon-knobs'

// Components
import Settings from './settings/settings'
import { SiteBanner, Tip, PanelWelcome, WalletPanel, WalletSummary, WalletSummarySlider, WalletWrapper } from '../../../src/features/rewards'
import WelcomePage from '../../../src/features/rewards/welcomePage'

const bartBaker = require('../../assets/img/bartBaker.jpeg')
const siteBgImage = require('../../assets/img/bg_siteBanner.jpg')
const siteBgLogo = require('../../assets/img/ddgo_siteBanner.svg')
const siteScreen = require('../../assets/img/ddgo_site.png')
const tipScreen = require('../../assets/img/tip_site.jpg')
const wallet = require('../../assets/img/rewards_wallet.svg')
const funds = require('../../assets/img/rewards_funds.svg')

const donationAmounts = [
  { tokens: 1, converted: 0.3, selected: false },
  { tokens: 5, converted: 1.5, selected: false },
  { tokens: 10, converted: 3, selected: false }
]

const dummyOptInAction = () => {
  console.log(dummyOptInAction)
}

storiesOf('Feature Components/Rewards/Concepts', module)
  .addDecorator(withKnobs)
  .add('Settings Page', () => <Settings />)
  .add('Welcome Page', () => (
    <WelcomePage
      id={'welcome-page'}
      optInAction={dummyOptInAction}
    />
  ))
  .add('Site Banner', withState({ donationAmounts, currentAmount: 5, showBanner: true }, (store) => {
    const onDonate = () => {
      console.log('onDonate')
    }

    const onAmountSelection = (tokens: number) => {
      store.set({ currentAmount: tokens })
    }

    const showBanner = () => {
      store.set({ showBanner: true })
    }

    const onClose = () => {
      store.set({ showBanner: false })
    }

    return (
      <div style={{ background: `url(${siteScreen}) no-repeat top center / cover`, width: '100%', height: '100vh' }}>
        <button onClick={showBanner}>Show banner</button>
        {
          store.state.showBanner
          ? <SiteBanner
            domain={text('Domain', 'duckduckgo.com')}
            title={text('Title', '')}
            currentDonation={number('Current recurring donation', 0)}
            balance={number('Balance ', 5)}
            bgImage={boolean('Show bg image', false) ? siteBgImage : null}
            logo={boolean('Show logo', false) ? siteBgLogo : null}
            donationAmounts={object('Donations', store.state.donationAmounts)}
            logoBgColor={text('Logo bg color', '')}
            onDonate={onDonate}
            onAmountSelection={onAmountSelection}
            currentAmount={store.state.currentAmount}
            onClose={onClose}
            social={[
              {
                type: 'twitter',
                name: '@DuckDuckGo',
                handler: 'DuckDuckGo'
              },
              {
                type: 'youtube',
                name: 'duckduckgo',
                handler: 'UCm_TyecHNHucwF_p4XpeFkQ'
              },
              {
                type: 'twitch',
                name: 'duckDuckGo',
                handler: 'duckduckgo'
              }
            ]}
          />
          : null
        }

      </div>
    )
  }))
  .add('Tip', withState({ donationAmounts, currentAmount: 5, allow: false }, (store) => {
    const onDonate = () => {
      console.log('onDonate')
    }

    const onClose = () => {
      console.log('onClose')
    }

    const onAllow = (allow: boolean) => {
      store.set({ allow })
    }

    const onAmountSelection = (tokens: number) => {
      store.set({ currentAmount: tokens })
    }

    return (
      <div style={{ background: `url(${tipScreen}) no-repeat top center`, width: '986px', height: '912px', margin: '0 auto', position: 'relative' }}>
        <div style={{ position: 'absolute', bottom: '185px', left: '330px' }}>
          <Tip
            donationAmounts={object('Donations', store.state.donationAmounts)}
            title={text('Title', 'Bart Baker')}
            allow={boolean('Allow tips', store.state.allow)}
            provider={text('Provider', 'YouTube')}
            balance={5}
            onDonate={onDonate}
            onClose={onClose}
            onAllow={onAllow}
            onAmountSelection={onAmountSelection}
            currentAmount={store.state.currentAmount}
          />
        </div>
      </div>
    )
  }))
  .add('Pre Opt-In', () => {
    return (
      <div style={{ background: `url(${tipScreen}) no-repeat top center`, width: '986px', height: '912px', margin: '0 auto', position: 'relative' }}>
        <div style={{ position: 'absolute', top: '40px', left: '120px', width: '373px', minHeight: '446px' }}>
          <PanelWelcome
            variant={'one'}
            optInAction={dummyOptInAction}
          />
        </div>
        <div style={{ position: 'absolute', top: '40px', left: '565px', width: '373px', minHeight: '446px' }}>
          <PanelWelcome
            variant={'two'}
            optInAction={dummyOptInAction}
          />
        </div>
      </div>
    )
  })
  .add('Wallet Panel', withState({ showPanel: true, tipsEnabled: true, includeInAuto: true }, (store) => {
    const doNothing = () => {
      console.log('do nothing')
    }

    const onIncludeInAuto = () => {
      store.set({ includeInAuto: !store.state.includeInAuto })
    }

    const onToggleTips = () => {
      store.set({ tipsEnabled: !store.state.tipsEnabled })
    }

    const onToggleSummary = () => {
      store.set({ showPanel: !store.state.showPanel })
    }

    const getProp = (show: boolean, prop: string): any => {
      switch (prop) {
        case 'opacity':
          return show ? 1 : 0
          break
        case 'overflow':
          return show ? 'unset' : 'hidden'
          break
        case 'height':
          return show ? '100%' : 0
        default:
          return ''
      }
    }

    return (
      <div style={{ background: `url(${tipScreen}) no-repeat top center`, width: '986px', height: '912px', margin: '0 auto', position: 'relative' }}>
        <div style={{ position: 'absolute', bottom: '185px', left: '330px' }}>
          <WalletWrapper
            contentPadding={!store.state.showPanel}
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
            <div
              style={{
                transition: 'visibility 0s, opacity 0.5s linear',
                overflow: getProp(store.state.showPanel, 'overflow'),
                opacity: getProp(store.state.showPanel, 'opacity'),
                height: getProp(store.state.showPanel, 'height')
              }}
            >
              <WalletPanel
                id={'wallet-panel'}
                platform={'youtube'}
                publisherImg={bartBaker}
                publisherName={'Bart Baker'}
                monthlyAmount={10}
                isVerified={true}
                tipsEnabled={boolean('Tips enabled', store.state.tipsEnabled)}
                includeInAuto={boolean('Tips enabled', store.state.includeInAuto)}
                attentionScore={'17'}
                donationAmounts={
                  [5, 10, 15, 20, 30, 50, 100]
                }
                onToggleTips={onToggleTips}
                donationAction={doNothing}
                onAmountChange={doNothing}
                onIncludeInAuto={onIncludeInAuto}
              />
              <WalletSummarySlider
                title={true}
                id={'panel-slider'}
                onToggle={onToggleSummary}
              />
            </div>
            <div
              style={{
                transition: 'visibility 0s, opacity 0.5s linear',
                overflow: getProp(!store.state.showPanel, 'overflow'),
                opacity: getProp(!store.state.showPanel, 'opacity'),
                height: getProp(!store.state.showPanel, 'height')
              }}
            >
              <WalletSummarySlider
                title={false}
                id={'summar-slider'}
                onToggle={onToggleSummary}
              />
              <div style={{ padding: '0px 30px' }}>
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
            </div>
          </WalletWrapper>
        </div>
      </div>
    )
  }))
