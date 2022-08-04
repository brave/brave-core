import * as React from 'react'
import * as S from './style'
import { withKnobs } from '@storybook/addon-knobs'

import './locale'
import MainPanel from '../components/main-panel'
import ThemeProvider from '$web-common/BraveCoreThemeProvider'

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
        <ThemeProvider>
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
