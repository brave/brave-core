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
import { DisabledPanel, SiteBanner, Tip, PanelWelcome, WalletPanel, WalletSummary, WalletSummarySlider, WalletWrapper } from '../../../src/features/rewards'
import { BatColorIcon, WalletAddIcon } from '../../../src/components/icons'
import WelcomePage from '../../../src/features/rewards/welcomePage'
import { Notification } from '../../../src/features/rewards/walletWrapper'

const bartBaker = require('../../assets/img/bartBaker.jpeg')
const siteBgImage = require('../../assets/img/bg_siteBanner.jpg')
const siteBgLogo = require('../../assets/img/ddgo_siteBanner.svg')
const siteScreen = require('../../assets/img/ddgo_site.png')
const tipScreen = require('../../assets/img/tip_site.jpg')

const captchaDrop = require('../../assets/img/captchaDrop.png')

const doNothing = (id: string) => {
  console.log('nothing')
}

const donationAmounts = [
  { tokens: '1.0', converted: '0.30', selected: false },
  { tokens: '5.0', converted: '1.50', selected: false },
  { tokens: '10.0', converted: '3.00', selected: false }
]

const defaultGrant = {
  promotionId: 'test',
  altcurrency: 'none',
  probi: '',
  expiryTime: 0,
  captcha: '',
  hint: ''
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

storiesOf('Feature Components/Rewards/Concepts/Desktop', module)
  .addDecorator(withKnobs)
  .add('Settings Page', () => <Settings />)
  .add('Welcome Page', () => (
    <WelcomePage
      id={'welcome-page'}
      optInAction={dummyOptInAction}
    />
  ))
  .add('Site Banner', withState({ donationAmounts, currentAmount: '5.0', showBanner: true }, (store) => {
    const onDonate = () => {
      console.log('onDonate')
    }

    const onAmountSelection = (tokens: string) => {
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
          ? <div style={{ position: 'fixed', top: 0, left: 0, height: '100vh', width: '100%', backgroundColor: 'rgba(12,13,33,0.85)' }}>
            <SiteBanner
              domain={text('Domain', 'duckduckgo.com')}
              name={text('Name', 'duckduckgo.com')}
              title={text('Title', '')}
              recurringDonation={boolean('Current recurring donation', true)}
              balance={text('Balance ', '5.0')}
              bgImage={boolean('Show bg image', false) ? siteBgImage : null}
              logo={boolean('Show logo', false) ? siteBgLogo : null}
              donationAmounts={object('Donations', store.state.donationAmounts)}
              logoBgColor={text('Logo bg color', '')}
              onDonate={onDonate}
              onAmountSelection={onAmountSelection}
              currentAmount={store.state.currentAmount}
              onClose={onClose}
              provider={select('Provider', { youtube: 'youtube', twitter: 'twitter', twitch: 'twitch' }, 'youtube')}
              social={[
                {
                  type: 'twitter',
                  url: 'https://twitter.com/DuckDuckGo'
                },
                {
                  type: 'youtube',
                  url: 'https://www.youtube.com/channel/UCm_TyecHNHucwF_p4XpeFkQ'
                },
                {
                  type: 'twitch',
                  url: 'https://www.twitch.tv/duckduckgo'
                }
              ]}
              showUnVerifiedNotice={boolean('Show unverified notice', false)}
            />
          </div>
          : null
        }
      </div>
    )
  }))
  .add('Tip', withState({ donationAmounts, currentAmount: '5.0', allow: false }, (store) => {
    const onDonate = () => {
      console.log('onDonate')
    }

    const onClose = () => {
      console.log('onClose')
    }

    const onAllow = (allow: boolean) => {
      store.set({ allow })
    }

    const onAmountSelection = (tokens: string) => {
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
            balance={text('Balance', '5')}
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
  .add('Pre Opt-In', withState({ creatingOne: false, creatingTwo: false }, (store) => {
    const creatingOne = () => {
      store.set({ creatingOne: true })
    }

    const creatingTwo = () => {
      store.set({ creatingTwo: true })
    }

    return (
      <div style={{ background: `url(${tipScreen}) no-repeat top center`, width: '986px', height: '912px', margin: '0 auto', position: 'relative' }}>
        <div style={{ position: 'absolute', top: '40px', left: '120px', width: '373px', minHeight: '446px', borderRadius: '8px', overflow: 'hidden' }}>
          <PanelWelcome
            variant={'one'}
            creating={store.state.creatingOne}
            optInAction={creatingOne}
            moreLink={dummyOptInAction}
            optInErrorAction={dummyOptInAction}
            error={boolean('Wallet Creation Error', false)}
          />
        </div>
        <div style={{ position: 'absolute', top: '40px', left: '565px', width: '373px', minHeight: '446px', borderRadius: '8px', overflow: 'hidden' }}>
          <PanelWelcome
            variant={'two'}
            optInAction={creatingTwo}
            creating={store.state.creatingTwo}
            optInErrorAction={dummyOptInAction}
            error={boolean('Wallet Creation Error', false)}
          />
        </div>
      </div>
    )
  }))
  .add('Disabled Panel', () => {
    const onPrivateLink = () => {
      console.log('open up private tab info')
    }

    const onNonPrivateLink = () => {
      console.log('open up rewards settings')
    }

    return (
      <div style={{ background: `url(${tipScreen}) no-repeat top center`, width: '986px', height: '912px', margin: '0 auto', position: 'relative' }}>
        <div style={{ position: 'absolute', top: '40px', left: '40px', width: '330px', borderRadius: '8px', overflow: 'hidden' }}>
          <DisabledPanel onLinkOpen={onNonPrivateLink} />
        </div>
        <div style={{ position: 'absolute', top: '40px', left: '460px', width: '330px', borderRadius: '8px', overflow: 'hidden' }}>
          <DisabledPanel isPrivate={true} onLinkOpen={onPrivateLink} />
        </div>
      </div>
    )
  })
  .add('Wallet Panel', withState({ grant: defaultGrant, notification: grantNotification, showSummary: false, tipsEnabled: true, includeInAuto: true }, (store) => {
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

    const onSolution = (x: number, y: number) => {
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

    const convertProbiToFixed = (probi: string, places: number = 1) => {
      return '0.0'
    }

    return (
      <div style={{ background: `url(${tipScreen}) no-repeat top center`, width: '986px', height: '100vh', margin: '0 auto', position: 'relative' }}>
        <div style={{ position: 'absolute', top: '50px', left: '560px', borderRadius: '8px', overflow: 'hidden' }}>
          <WalletWrapper
            compact={true}
            contentPadding={false}
            notification={store.state.notification as Notification}
            gradientTop={getGradientColor()}
            balance={text('Tokens', '30.0')}
            converted={text('Converted', '15.50 USD')}
            actions={[
              {
                name: 'Add funds',
                action: doNothing,
                icon: <WalletAddIcon />
              },
              {
                name: 'Settings',
                action: doNothing,
                icon: <BatColorIcon />
              }
            ]}
            showCopy={boolean('Show Uphold', false)}
            showSecActions={false}
            connectedWallet={boolean('Connected wallet', false)}
            grants={object('Grants', [
              {
                tokens: '8.0',
                expireDate: '7/15/2018'
              },
              {
                tokens: '10.0',
                expireDate: '9/10/2018'
              },
              {
                tokens: '10.0',
                expireDate: '10/10/2018'
              }
            ])}
            grant={store.state.grant}
            onGrantHide={onGrantHide}
            onFetchCaptcha={onFetchCaptcha}
            onSolution={onSolution}
            onFinish={onFinish}
            convertProbiToFixed={convertProbiToFixed}
          >
            <WalletSummarySlider
              id={'panel-slider'}
              onToggle={onSummaryToggle}
            >
              <WalletPanel
                id={'wallet-panel'}
                toggleTips={boolean('Toggle tips', true)}
                platform={'youtube'}
                publisherImg={bartBaker}
                publisherName={'Bart Baker'}
                monthlyAmount={'5.0'}
                isVerified={true}
                tipsEnabled={boolean('Tips enabled', store.state.tipsEnabled)}
                includeInAuto={boolean('Tips enabled', store.state.includeInAuto)}
                attentionScore={'17'}
                donationAmounts={
                  [
                    {
                      tokens: '0.0',
                      converted: '0.00'
                    },
                    {
                      tokens: '1.0',
                      converted: '0.50'
                    },
                    {
                      tokens: '5.0',
                      converted: '2.50'
                    },
                    {
                      tokens: '10.0',
                      converted: '5.00'
                    }
                  ]
                }
                onToggleTips={onToggleTips}
                donationAction={doNothing}
                onAmountChange={doNothing}
                onIncludeInAuto={onIncludeInAuto}
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
      </div>
    )
  }))
storiesOf('Feature Components/Rewards/Concepts/Mobile', module)
  .add('Welcome Page', () => (
    <WelcomePage
      id={'welcome-page'}
      optInAction={dummyOptInAction}
    />
  ))
  .add('Settings', () => <SettingsMobile />)
  .add('Site Banner', withState({ donationAmounts, currentAmount: '5.0', showBanner: true }, (store) => {
    const onDonate = () => {
      console.log('onDonate')
    }

    const onAmountSelection = (tokens: string) => {
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
          ? <div style={{ position: 'fixed', top: 0, left: 0, height: '100vh', width: '100%', backgroundColor: 'rgba(12,13,33,0.85)' }}>
            <SiteBanner
              isMobile={true}
              domain={text('Domain', 'duckduckgo.com')}
              title={text('Title', '')}
              recurringDonation={false}
              balance={text('Balance ', '5.0')}
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
                  url: 'https://twitter.com/DuckDuckGo'
                },
                {
                  type: 'youtube',
                  url: 'https://www.youtube.com/channel/UCm_TyecHNHucwF_p4XpeFkQ'
                },
                {
                  type: 'twitch',
                  url: 'https://www.twitch.tv/duckduckgo'
                }
              ]}
            />
          </div>
          : null
        }
      </div>
    )
  }))
