import * as React from 'react'
import * as S from './style'
import { withKnobs } from '@storybook/addon-knobs'

import './locale'
import MainPanel from '../components/main-panel'
import ThemeProvider from '$web-common/BraveCoreThemeProvider'
import darkTheme from '../theme/dark'
import lightTheme from '../theme/light'

export default {
  title: 'Speedreader/Panels',
  parameters: {
    layout: 'centered'
  },
  argTypes: {
    locked: { control: { type: 'boolean', lock: false } }
  },
  decorators: [
    (Story: any) => {
      return (
        <ThemeProvider
          dark={darkTheme}
          light={lightTheme}
        >
          <Story />
        </ThemeProvider>
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
