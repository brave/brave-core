/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Image from 'brave-ui/old/v1/image'
import { PushButtonLink } from 'brave-ui/old/v1/pushButton'
import { Heading, Paragraph } from 'brave-ui/old'

// Constants
import { theme } from '../constants/theme'

// Utils
import { getLocale } from '../../common/locale'

// Assets
const featuresImage = require('../../img/welcome/features.png')

export default class FeaturesScreen extends React.PureComponent<{}, {}> {
  render () {
    return (
      <section style={theme.content}>
        <Image customStyle={theme.featuresImage} src={featuresImage} />
        <Heading level={1} customStyle={theme.title} text={getLocale('customizePreferences')} />
        <Paragraph customStyle={theme.text} text={getLocale('configure')} />
        <PushButtonLink
          color='primary'
          size='large'
          customStyle={theme.mainButton}
          href='chrome://settings'
        >
          {getLocale('preferences')}
        </PushButtonLink>
      </section>
    )
  }
}
