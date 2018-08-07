/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { DisabledContent, Box } from '../../../../src/features/rewards'

// Utils
import locale from './fakeLocale'

// Assets
const adsImg = require('../../../assets/img/rewards_ads.svg')

class AdsBox extends React.Component {
  adsDisabled () {
    return (
      <DisabledContent
        image={adsImg}
        theme={{ color: '#ceb4e1', boldColor: '#b490cf' }}
      >
        <h3>Coming soon.</h3>
      </DisabledContent>
    )
  }

  render () {
    return (
      <Box
        title={locale.adsTitle}
        theme={{ titleColor: '#C12D7C' }}
        description={locale.adsDesc}
        toggle={false}
        disabledContent={this.adsDisabled()}
      />
    )
  }
}

export default AdsBox
