/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { storiesOf } from '@storybook/react'
import { boolean, withKnobs } from '@storybook/addon-knobs'
import { Provider as ReduxProvider } from 'react-redux'
import ftxState from './default/data/ftxData'

import store from '../../../store'

// Decorators
function StoreProvider ({ story }: any) {
  return (
    <ReduxProvider store={store}>
     {story}
    </ReduxProvider>
  )
}

// Components
import { FTXWidget as FTX } from '../ftx'

// Stubbed functions
const doNothing = () => undefined

storiesOf('New Tab/Widgets', module)
  .addDecorator(withKnobs)
  .addDecorator(story => <StoreProvider story={story()} />)
  .add('FTX', () => {
    return (
      <div style={{ width: 284 }}>
        <FTX
          {...ftxState}
          widgetTitle={'FTX'}
          paddingType={'none'}
          menuPosition={'left'}
          textDirection={'left'}
          showContent={boolean('Show content?', true)}
          stackPosition={1}
          onShowContent={doNothing}
        />
      </div>
    )
  })
