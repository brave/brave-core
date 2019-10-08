/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { storiesOf } from '@storybook/react'
import welcomeDarkTheme from '../theme/welcome-dark'
import welcomeLightTheme from '../theme/welcome-light'
import { withThemesProvider } from 'storybook-addon-styled-component-theme'
import { withKnobs, boolean } from '@storybook/addon-knobs'

// Components
import WelcomePage from './page/index'

// Themes
const themes = [welcomeLightTheme, welcomeDarkTheme]

const fullPageStoryStyles: object = {
  width: '100%',
  height: '100%'
}

export const FullPageStory = (storyFn: any) =>
  <div style={fullPageStoryStyles}>{storyFn()}</div>

storiesOf('Welcome', module)
  .addDecorator(withThemesProvider(themes))
  .addDecorator(withKnobs)
  .addDecorator(FullPageStory)
  .add('Page', () => {
    return (
      <WelcomePage isDefaultSearchGoogle={boolean('Is default search google?', true)}/>
    )
  })
