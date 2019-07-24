/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { storiesOf } from '@storybook/react'
import { withKnobs, boolean } from '@storybook/addon-knobs'

// Components
import WelcomePage from './page/index'

const fullPageStoryStyles: object = {
  width: '-webkit-fill-available',
  height: '-webkit-fill-available'
}

export const FullPageStory = (storyFn: any) =>
  <div style={fullPageStoryStyles}>{storyFn()}</div>

storiesOf('Welcome', module)
  .addDecorator(withKnobs)
  .addDecorator(FullPageStory)
  .add('Page', () => {
    return (
      <WelcomePage isDefaultSearchGoogle={boolean('Is default search google?', true)}/>
    )
  })
