/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { storiesOf } from '@storybook/react'
import { withKnobs } from '@storybook/addon-knobs'

// Components
import { NewWidgetTooltip } from '../shared/newWidgetTooltip'
import ftxLogo from '../ftx/ftx-logo.png'
import bannerImage from '../../../containers/newTab/settings/assets/crypto-dot-com.png'

storiesOf('New Tab/Widgets', module)
  .addDecorator(withKnobs)
  .add('New Widget Tooltip', () => {
    return (
      <div style={{ width: 284 }}>
        <NewWidgetTooltip
          widgetInfo={{
            title: 'FTX',
            icon: ftxLogo,
            description: 'Trade and sell cryptocurrency on this popular exchange.',
            bannerImage: bannerImage
          }}
        />
      </div>
    )
  })
