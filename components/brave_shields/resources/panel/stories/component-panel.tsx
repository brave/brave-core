import * as React from 'react'
import * as S from './style'
import { withKnobs } from '@storybook/addon-knobs'

import './locale'
import MainPanel from '../components/main-panel'
import shieldsDarkTheme from '../theme/shields-dark'
import shieldsLightTheme from '../theme/shields-light'
import ThemeProvider from '../../../../common/StorybookThemeProvider'

export default {
  title: 'ShieldsV2/Panels',
  parameters: {
    layout: 'centered'
  },
  argTypes: {
    locked: { control: { type: 'boolean', lock: false } }
  },
  decorators: [
    (Story: any) => <ThemeProvider darkTheme={shieldsDarkTheme}
    lightTheme={shieldsLightTheme}><Story /></ThemeProvider>,
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
