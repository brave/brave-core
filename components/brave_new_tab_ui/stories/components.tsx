// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { storiesOf } from '@storybook/react'
import { withKnobs, select, text } from '@storybook/addon-knobs'
import Theme from 'brave-ui/theme/brave-default'
import DarkTheme from 'brave-ui/theme/brave-dark'
import BraveCoreThemeProvider from '../../common/BraveCoreThemeProvider'

// Components
import Loading from '../components/loading'
import BraveTodayLoadingCard from '../components/default/braveToday/cards/cardLoading'
import BraveTodayErrorCard from '../components/default/braveToday/cards/cardError'
import PublisherMeta from '../components/default/braveToday/cards/PublisherMeta'

function ThemeProvider ({ story }: any) {
  const themeName = select(
    'Theme',
    { ['Light']: 'Light', ['Dark']: 'Dark' },
    'Light'
  )
  console.log('theme', themeName)
  return (
    <BraveCoreThemeProvider
      dark={DarkTheme}
      light={Theme}
      initialThemeType={themeName as chrome.braveTheme.ThemeType}
    >
    {story}
    </BraveCoreThemeProvider>
  )
}

const onClick = (() => alert('clicked'))

storiesOf('New Tab', module)
  .addDecorator(withKnobs)
  .addDecorator(story => <ThemeProvider story={story()} />)
  .add('Loading', () => (
    <div
      style={{ width: '500px', height: '500px', display: 'flex', alignItems: 'center', justifyContent: 'center' }}
    >
      <Loading />
    </div>
  ))

storiesOf('New Tab/Brave Today', module)
  .addDecorator(withKnobs)
  .addDecorator(story => <ThemeProvider story={story()} />)
  .addDecorator(story => (
    <div
      style={{
        display: 'flex',
        fontFamily: 'Poppins',
        alignItems: 'center',
        justifyContent: 'center',
        gap: '20px',
        width: '100%',
        minHeight: '100vh',
        background: 'url(https://placekitten.com/2000/3000)',
        backgroundSize: 'contain',
        color: 'pink',
        fontSize: '22px',
        fontWeight: 600
      }}
    >
      {story()}
    </div>
  ))
  .add('Publisher', () => (
    <>
      <PublisherMeta
        publisher={{
          publisher_id: '123abc',
          publisher_name: text('Publisher Name', 'small'),
          category: 'Top News',
          enabled: true,
          user_enabled: null
        }}
        onSetPublisherPref={onClick}
      />
      <PublisherMeta
        publisher={{
          publisher_id: '123abcdef',
          publisher_name: text('Publisher Name 2', 'The Miller Chronicle'),
          category: 'Top News',
          enabled: true,
          user_enabled: null
        }}
        onSetPublisherPref={onClick}
      />
    </>
  ))
  .add('Loading', () => (
    <BraveTodayLoadingCard />
  ))
  .add('Error', () => (
    <BraveTodayErrorCard />
  ))
