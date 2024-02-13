import 'emptykit.css'
import * as React from 'react'
import { withKnobs, boolean } from '@storybook/addon-knobs'
import {setIconBasePath} from '@brave/leo/shared/icon'
import '../components/web-components/app.global.scss'
import { getString } from './locale'
import ThemeProvider from '../components/common/BraveCoreThemeProvider'

// Fonts
import '../ui/webui/resources/fonts/poppins.css'
import '../ui/webui/resources/fonts/manrope.css'
import '../ui/webui/resources/fonts/inter.css'

// Icon path
// The storybook might be hosted at the root, but it might also be hosted
// somewhere deep. The icons will be hosted in the relative path of the
// storybook. Let's find the relative path we're at, and give that to
// Nala icons.
if (!document.location.pathname.endsWith('/iframe.html')) {
  // Perhaps storybook was upgraded and this changed?
  console.error('Could not ascertain path that the storybook is hosted at. Not able to set static icon path!')
} else {
  const storybookPath = document.location.pathname.substring(0, document.location.pathname.lastIndexOf('/'))
  setIconBasePath(`${storybookPath}/icons`)
}

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
  getBoolean(key) {
    return false
  }
}
chrome.extension = {
  inIncognitoContext: false
}

export const decorators = [
  (Story) => (
    <div dir={boolean('rtl?', false) ? 'rtl' : ''}>
      <Story />
    </div>
  ),
  (Story, context) => (
    <ThemeProvider
      dark={context.args.darkTheme}
      light={context.args.lightTheme}
    >
      <Story />
    </ThemeProvider>
  ),
  withKnobs
]
