/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { withState } from '@dump247/storybook-state'
import { storiesOf } from '@storybook/react'
import { withKnobs, boolean, number, text, object, select } from '@storybook/addon-knobs'

// Components
import Settings from './settings/settings'
import SettingsMobile from './settingsMobile/settingsMobile'
import {
  DisabledPanel,
  MediaBox,
  SiteBanner,
  Tip,
  WalletPanel,
  WalletSummary,
  WalletSummarySlider,
  WalletWrapper
} from '../components'
import { BatColorIcon, WalletAddIcon } from 'brave-ui/components/icons'
import { Notification, WalletState } from '../components/walletWrapper'

const favicon = require('./img/brave-favicon.png')
const siteBgImage = require('./img/bg_siteBanner.jpg')
const siteBgLogo = require('./img/ddgo_siteBanner.svg')
const siteScreen = require('./img/ddgo_site.png')

const captchaDrop = require('./img/captchaDrop.png')

const doNothing = (id: string) => {
  console.log('nothing')
}

const donationAmounts = [
  { tokens: '1.0', converted: '0.30', selected: false },
  { tokens: '5.0', converted: '1.50', selected: false },
  { tokens: '10.0', converted: '3.00', selected: false }
]

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
  .add('Site Banner', withState({ donationAmounts, currentAmount: '5.0', showBanner: true }, (store) => {
    const mediaProvider = select<any>('Provider', { youtube: 'youtube', twitter: 'twitter', twitch: 'twitch', reddit: 'reddit', github: 'github' }, 'youtube')
    const screenName = text('Screen Name', '')
    const commentText = text('Post Text', '')

    const onDonate = () => {
      console.log('onDonate')
    }

    const onTweet = () => {
      console.log('onTweet')
    }

    const onAmountSelection = (tokens: string) => {
      store.set({ currentAmount: tokens })
    }

    const showBanner = () => {
      store.set({ showBanner: true })
    }

    const isTwitterTip = () => {
      return mediaProvider === 'twitter'
    }

    const isRedditTip = () => {
      return mediaProvider === 'reddit'
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
                onTweet={onTweet}
                type={select('Banner Type', {
                  'one-time': 'one-time',
                  monthly: 'monthly'
                }, 'one-time')}
                domain={text('Domain', 'duckduckgo.com')}
                name={text('Name', 'duckduckgo.com')}
                screenName={screenName}
                title={text('Title', '')}
                balance={text('Balance ', '5.0')}
                bgImage={boolean('Show bg image', false) ? siteBgImage : null}
                logo={boolean('Show logo', false) ? siteBgLogo : null}
                donationAmounts={object('Donations', store.state.donationAmounts)}
                logoBgColor={text('Logo bg color', '')}
                onDonate={onDonate}
                onAmountSelection={onAmountSelection}
                currentAmount={store.state.currentAmount}
                onClose={onClose}
                provider={mediaProvider}
                nextContribution={'07/08/2019'}
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
                  },
                  {
                    type: 'reddit',
                    url: 'https://www.reddit.com/r/duckduckgo'
                  },
                  {
                    type: 'github',
                    url: 'https://github.com/duckduckgo'
                  }
                ]}
                showUnVerifiedNotice={boolean('Show unverified notice', false)}
                isVerified={boolean('Is publisher verified', true)}
              >
                {
                  isTwitterTip()
                  ? <MediaBox
                      mediaType={'twitter'}
                      mediaText={commentText}
                      mediaTimestamp={number('Timestamp in seconds', 46420000) || 0}
                  />
                  : isRedditTip()
                  ? <MediaBox
                      mediaType={'reddit'}
                      mediaText={commentText}
                      mediaTimestamp={0}
                      mediaTimetext={'3 days ago'}
                  />
                  : null
                }
              </SiteBanner>
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
      <div>
        <div>
          <Tip
            donationAmounts={object('Donations', store.state.donationAmounts)}
            title={text('Title', 'Jonathon Doe')}
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
          onlyAnonWallet={boolean('Anon Wallet Only', false)}
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
              onlyAnonWallet={boolean('Anon Wallet Only', false)}
            />
          </WalletSummarySlider>
        </WalletWrapper>
      </div>
    )
  }))
storiesOf('Rewards/Concepts/Mobile', module)
  .add('Settings', () => <SettingsMobile />)
