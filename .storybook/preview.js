import 'emptykit.css'
import * as React from 'react'
import { withKnobs, boolean } from '@storybook/addon-knobs'
import { addParameters } from '@storybook/react'
import { initLocale } from 'brave-ui/helpers'
import '../components/web-components/app.global.scss'
import { getString } from './locale'
import ThemeProvider from '../components/common/BraveCoreThemeProvider'

// Fonts
import '../ui/webui/resources/fonts/poppins.css'

export const parameters = {
  backgrounds: {
    default: 'Dynamic',
    values: [
      { name: 'Dynamic', value: 'var(--background1)' },
      { name: 'Neutral300', value: '#DEE2E6' },
      { name: 'Grey700', value: '#5E6175' },
      { name: 'White', value: '#FFF' },
      { name: 'Grey900', value: '#1E2029' }
    ]
  }
}

window.loadTimeData = {
  getString,
  getBoolean (key) {
    return false
  }
}
chrome.extension = {
  inIncognitoContext: false
}

export const decorators = [
  (Story) => <div dir={boolean('rtl?', false) ? 'rtl' : ''}><Story /></div>,
  (Story, context) => <ThemeProvider dark={context.args.darkTheme} light={context.args.lightTheme}><Story /></ThemeProvider>,
  withKnobs
]
