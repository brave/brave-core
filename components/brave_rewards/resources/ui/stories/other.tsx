/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { withState } from '@dump247/storybook-state'
import { storiesOf } from '@storybook/react'
import { withKnobs, boolean, select, text, object, number } from '@storybook/addon-knobs'
// @ts-ignore
import centered from '@storybook/addon-centered/dist'
// Components
import {
  Box,
  DisabledContent,
  Alert,
  MainToggle,
  Donate,
  List,
  ListToken,
  Tokens,
  Profile,
  Amount,
  PanelWelcome,
  ToggleTips,
  Tooltip,
  DonationOverlay
} from '../../../src/features/rewards'
import {
  BoxMobile
} from '../../../src/features/rewards/mobile'
import { BatColorIcon, SettingsIcon, UpholdColorIcon } from '../../../src/components/icons'
import GrantClaim from '../../../src/features/rewards/grantClaim'

const donate = require('../../assets/img/rewards_donate.svg')
const bart = require('../../assets/img/bartBaker.jpeg')
const tipScreen = require('../../assets/img/tip_site.jpg')

const dummyClick = () => {
  console.log(dummyClick)
}

const donationAmounts = [
  { tokens: '1.0', converted: '0.30', selected: false },
  { tokens: '5.0', converted: '1.50', selected: false },
  { tokens: '10.0', converted: '3.00', selected: false }
]

storiesOf('Feature Components/Rewards/Other/Desktop', module)
  .addDecorator(withKnobs)
  .addDecorator(centered)
  .add('Box', withState({ checked: false, toggle: true }, (store) => {
    const onToggle = () => {
      store.set({ checked: !store.state.checked })
    }
    return (
      <div style={{ width: '595px' }}>
      <Box
        title={text('Title', 'Brave ads')}
        toggle={boolean('Show toggle', store.state.toggle)}
        checked={boolean('Toggle checked', store.state.checked)}
        type={select('Type', { contribute: 'contribute', donation: 'donation', ads: 'ads' }, 'contribute')}
        description={
          text('Description', `Earn tokens by seeing ads on Brave. Ads are matched
          from machine learning and the data temporarily present in your browser without tracking your
          information or sending it outside.`)
        }
        onToggle={onToggle}
        settingsChild={<div>Settings content</div>}
      >
        <div>Some content</div>
      </Box>
    </div>
    )
  }))
  .add('Disabled content',() => {
    return (
      <DisabledContent
        image={donate}
        type={'donation'}
      >
        • Donate on the spot as you find gems. <br/>
        • <b>Enable Tips </b> on Twitter, YouTube, and more, to give tips to posts you ‘Like’.
      </DisabledContent>
    )
  })
  .add('Alert',() => {
    return (
      <Alert
        type={select('Type', { error: 'error', success: 'success', warning: 'warning' }, 'success')}
        bg={boolean('Background color', false)}
        colored={boolean('Text color', false)}
      >
        <b>Funds received!</b> 25 BAT are added to your wallet successfully.
      </Alert>
    )
  })
  .add('Main toggle', withState({ checked: false }, (store) => {
    const onToggle = () => {
      store.set({ checked: !store.state.checked })
    }
    return (
      <div style={{ width: '800px' }}>
        <MainToggle
          enabled={boolean('Enable', store.state.checked)}
          onToggle={onToggle}
        />
      </div>
    )
  }))
  .add('Donate', withState({ donationAmounts, currentAmount: 5 }, (store) => {
    const onDonate = () => {
      console.log('onDonate')
    }

    const onAmountSelection = (tokens: number) => {
      store.set({ currentAmount: tokens })
    }

    return (
      <div style={{ background: '#696fdc' }}>
      <Donate
        donateType={select('Type', { big: 'big', small: 'small' }, 'small')}
        balance={number('Balance ', 5)}
        donationAmounts={object('Donations', store.state.donationAmounts)}
        onDonate={onDonate}
        title={'Donation amount'}
        actionText={text('Action text', 'Send my Donation')}
        onAmountSelection={onAmountSelection}
        currentAmount={number('Current amount', store.state.currentAmount)}
      />
    </div>
    )
  }))
    .add('List',() => {
      return (
        <div style={{ width: '595px' }}>
          <List
            title={text('Title', 'Earnings this month')}
          >
            Some content
          </List>
        </div>
      )
    })
    .add('List - Token',() => {
      return (
        <div style={{ width: '400px' }}>
          <ListToken
            title={text('Title', 'Brave Contribute')}
            value={text('Value', '10.0')}
            converted={text('Converted', '0.25')}
            isNegative={boolean('Is negative', false)}
            color={select('Color', { contribute: 'contribute', donation: 'donation', earnings: 'earnings', notPaid: 'notPaid', default: 'default' }, 'default')}
            size={select('Size', { normal: 'normal', small: 'small' }, 'small')}
          />
        </div>
      )
    })
    .add('Tokens',() => {
      return (
        <Tokens
          value={text('Tokens value', '10.0')}
          converted={text('Converted value', '4.00')}
          currency={text('Currency', 'USD')}
          isNegative={boolean('Is negative', false)}
          color={select('Color', { contribute: 'contribute', donation: 'donation', earnings: 'earnings', notPaid: 'notPaid', default: 'default' }, 'default')}
          size={select('Size', { normal: 'normal', small: 'small' }, 'small')}
        />
      )
    })
    .add('Profile',() => {
      return (
        <div style={{ width: '400px' }}>
          <Profile
            type={select('Type', { big: 'big', small: 'small' }, 'big')}
            title={'Bart Baker'}
            verified={boolean('Verified', false)}
            provider={select('Provider', { youtube: 'YouTube', twitter: 'Twitter', twitch: 'Twitch' }, 'youtube')}
            src={bart}
          />
        </div>
      )
    })
    .add('Amount', withState({ selected: false }, (store) => {
      const onSelect = () => {
        store.set({ selected: !store.state.selected })
      }

      return (
        <div style={{ background: '#696fdc', width: '335px', padding: '50px' }}>
          <Amount
            amount={text('Amount', '5.0')}
            converted={text('Converted', '1.50')}
            selected={boolean('Selected', store.state.selected)}
            type={select('Type', { big: 'Big', small: 'Small' }, 'big')}
            onSelect={onSelect}
          />
        </div>
      )
    }))
    .add('Grant claim',() => {
      return (
        <GrantClaim
          onClaim={dummyClick}
        />
      )
    })
    .add('Panel Welcome', () => {
      return (
        <div style={{ width: '373px', minHeight: '446px' }}>
          <PanelWelcome
            optInAction={dummyClick}
            variant={select('Variant', { one: 'One', two: 'Two' }, 'one')}
          />
        </div>
      )
    })
    .add('Toggle Tips', withState({ tipsEnabled: true }, (store) => {
      const onToggle = () => {
        store.set({ tipsEnabled: !store.state.tipsEnabled })
      }
      return (
        <ToggleTips
          id={'toggle-tips'}
          onToggleTips={onToggle}
          tipsEnabled={boolean('Selected', store.state.tipsEnabled)}
          provider={select('Provider', { youtube: 'YouTube', twitter: 'Twitter', twitch: 'Twitch' }, 'youtube')}
        />
      )
    }))
    .add('Tooltip', () => {
      const braveAdsText = <span>Brave Ads Settings</span>
      const batLogoText = <span>BAT Logo</span>
      const styledUpholdContent = (
        <div>
          <span style={{ fontWeight: 'bold', color: '#4AAF57' }}>
            Uphold. The Internet of Money.
          </span>
        </div>
      )

      return (
        <div>
          <div style={{ position: 'absolute', top: '200px', left: '700px' }}>
            <Tooltip
              id={'tooltip-test'}
              content={braveAdsText}
            >
              <div style={{ width: '30px' }}>
                <SettingsIcon color={'#A1A8F2'}/>
              </div>
            </Tooltip>
          </div>
          <div style={{ position: 'absolute', top: '280px', left: '685px' }}>
            <Tooltip
              id={'tooltip-test'}
              content={batLogoText}
            >
              <div style={{ width: '60px' }}>
                <BatColorIcon/>
              </div>
            </Tooltip>
          </div>
          <div style={{ position: 'absolute', top: '390px', left: '670px' }}>
            <Tooltip
              id={'tooltip-test'}
              content={styledUpholdContent}
            >
              <div style={{ width: '90px' }}>
                <UpholdColorIcon/>
              </div>
            </Tooltip>
          </div>
        </div>
      )
    })
    .add('Donation Overlay', withState({ displayed: true }, (store) => {
      const onOverlayClose = () => {
        store.set({ displayed: false })
      }

      return (
        <div style={{ background: `url(${tipScreen}) no-repeat top center`, width: '986px', height: '100vh', margin: '0 auto', position: 'relative' }}>
          {
            store.state.displayed
            ? <DonationOverlay
                siteImg={bart}
                onClose={onOverlayClose}
                success={boolean('Success', true)}
            />
            : null
          }
        </div>
      )
    }))
storiesOf('Feature Components/Rewards/Other/Mobile', module)
  .addDecorator(withKnobs)
  .addDecorator(centered)
  .add('Box', withState({ checked: true, toggle: true }, (store) => {
    const onToggle = () => {
      store.set({ checked: !store.state.checked })
    }

    return (
      <div style={{ width: '100%' }}>
        <BoxMobile
          title={text('Title', 'Brave Auto-Contribute')}
          toggle={boolean('Show toggle', store.state.toggle)}
          checked={boolean('Toggle checked', store.state.checked)}
          type={select('Type', { contribute: 'contribute', donation: 'donation', ads: 'ads' }, 'contribute')}
          description={
            text('Description', `A simple way to support content creators. Set monthly allowance and browse normally. Your favorite sites (content sites only) receive your contributions automatically. You can exclude sites from funding right from the list below.`)
          }
          toggleAction={onToggle}
          settingsChild={<div>Settings content</div>}
        >
          <div style={{ padding: '0px 25px' }}>
            <p>Detail Content</p>
          </div>
        </BoxMobile>
      </div>
    )
  }))
