import * as React from 'react'
import * as S from './style'
import { withKnobs } from '@storybook/addon-knobs'

import './locale'
import MainPanel from '../components/main-panel'
import shieldsDarkTheme from '../theme/shields-dark'
import shieldsLightTheme from '../theme/shields-light'
import ThemeProvider from '../../../../common/StorybookThemeProvider'
import DataContext from '../state/context'
import { AdBlockMode, FingerprintMode, CookieBlockMode } from '../api/panel_browser_api'

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
          host: 'brave.com',
          totalBlockedResources: 2,
          isShieldsEnabled: true,
          adsList: [{ url: 'ads.brave.com' }, { url: 'ads2.brave.com' }],
          jsList: [],
          httpRedirectsList: [],
          fingerprintsList: []
        },
        siteSettings: {
          adBlockMode: AdBlockMode.ALLOW,
          fingerprintMode: FingerprintMode.ALLOW,
          cookieBlockMode: CookieBlockMode.ALLOW,
          isHttpsEverywhereEnabled: true,
          isNoscriptEnabled: false
        }
      }

      return (
        <DataContext.Provider value={store}>
          <ThemeProvider
            darkTheme={shieldsDarkTheme}
            lightTheme={shieldsLightTheme}
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
