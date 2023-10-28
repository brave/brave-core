// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import * as S from './style'
import { withKnobs, boolean, text } from '@storybook/addon-knobs'

import './locale'
import MainPanel from '../components/main-panel'
import TreeList from '../components/tree-list'
import shieldsDarkTheme from '../theme/shields-dark'
import shieldsLightTheme from '../theme/shields-light'
import ThemeProvider from '../../../../common/BraveCoreThemeProvider'
import DataContext from '../state/context'
import { AdBlockMode, FingerprintMode, CookieBlockMode, HttpsUpgradeMode } from '../api/panel_browser_api'
import {
  ViewType
} from '../state/component_types'
import { getLocale } from '../../../../common/locale'

const LIST_JS = [
  { 'url': 'https://www.reddit.com/' },
  { 'url': 'https://www.redditstatic.com/desktop2x/AuthorHovercard~Reddit.ca2d5405cdd178092347.js' },
  { 'url': 'https://www.redditstatic.com/desktop2x/Chat~Governance~Reddit.f64af7713c3261596c7c.js' },
  { 'url': 'https://www.redditstatic.com/desktop2x/CollectionCommentsPage~CommentsPage~ModerationPages~PostCreation~ProfileComments~ProfileOverview~Pro~d39c0d57.a37d209138f811a78126.js' },
  { 'url': 'https://www.redditstatic.com/desktop2x/CollectionCommentsPage~CommentsPage~ModerationPages~ProfileComments~ProfileOverview~ProfilePrivate~R~969c2956.44543839280e344fe35c.js' },
  { 'url': 'https://www.redditstatic.com/desktop2x/Frontpage.25b06963dc7114d6d5e5.js' },
  { 'url': 'https://www.redditstatic.com/desktop2x/Governance~ModListing~Reddit~ReportFlow~Subreddit.dcc2d2bd4fa3a45348eb.js' },
  { 'url': 'https://www.redditstatic.com/desktop2x/Governance~ModListing~Reddit~Subreddit.3f168f79abf6d859d163.js' },
  { 'url': 'https://www.redditstatic.com/desktop2x/Governance~Reddit.e7882cd59d01f560d9ed.js' },
  { 'url': 'https://www.redditstatic.com/desktop2x/Governance~Reddit~Subreddit~reddit-components-BlankPost~reddit-components-ClassicPost~reddit-compone~3b56c92e.14312dc2a044d45449c9.js' },
  { 'url': 'https://www.redditstatic.com/desktop2x/ModListing~PostCreation~Reddit~StandalonePostPage~Subreddit.b725e7ecf5d621c30c53.js' },
  { 'url': 'https://www.redditstatic.com/desktop2x/PostCreation~Reddit~StandalonePostPage~SubredditTopContent~TopWeekPostsDiscoveryUnit~reddit-componen~2583c786.1cc84f65074a696c0941.js' },
  { 'url': 'https://www.redditstatic.com/desktop2x/PostCreation~Reddit~StandalonePostPage~Subreddit~reddit-components-ClassicPost~reddit-components-Com~82e48dd3.85bb0a8c6be58f085676.js' },
  { 'url': 'https://api.github.com/_private/browser/stats' }
]

const LIST_ADS = [
  { url: 'ads.brave.com' },
  { url: 'ads2.brave.com' }
]

export default {
  title: 'ShieldsV2/Panels',
  parameters: {
    layout: 'centered'
  },
  argTypes: {
    locked: { control: { type: 'boolean', lock: false } }
  },
  decorators: [
    (Story: any) => {
      // mock data
      const store = {
        siteBlockInfo: {
          host: text('Host name', 'brave.com'),
          totalBlockedResources: (LIST_ADS.length + LIST_JS.length),
          isBraveShieldsEnabled: boolean('Enable Shields', true),
          isBraveShieldsManaged: boolean('Shields Managed', false),
          adsList: LIST_ADS,
          blockedJsList: LIST_JS,
          allowedJsList: LIST_JS,
          httpRedirectsList: [],
          fingerprintsList: [],
          faviconUrl: { url: 'https://brave.com/static-assets/images/brave-favicon.png' }
        },
        siteSettings: {
          adBlockMode: AdBlockMode.ALLOW,
          fingerprintMode: FingerprintMode.ALLOW,
          cookieBlockMode: CookieBlockMode.ALLOW,
          httpsUpgradeMode: HttpsUpgradeMode.DISABLED,
          isNoscriptEnabled: false,
          isForgetFirstPartyStorageEnabled: false
        },
        viewType: ViewType.Main
      }

      return (
        <DataContext.Provider value={store}>
          <ThemeProvider
            dark={shieldsDarkTheme}
            light={shieldsLightTheme}
          >
            <Story />
          </ThemeProvider>
        </DataContext.Provider>
      )
    },
    withKnobs
  ]
}

export const _Main = () => {
  return (
    <S.PanelFrame>
      <MainPanel />
    </S.PanelFrame>
  )
}

export const _ResourceList = () => {
  const { siteBlockInfo } = React.useContext(DataContext)

  if (!siteBlockInfo) {
    return
  }

  return (
    <S.PanelFrame>
      <TreeList
        blockedList={ siteBlockInfo?.blockedJsList }
        allowedList={ siteBlockInfo?.allowedJsList }
        totalAllowedTitle={getLocale('braveShieldsAllowedScriptsLabel')}
        totalBlockedTitle={getLocale('braveShieldsBlockedScriptsLabel')}
      />
    </S.PanelFrame>
  )
}
