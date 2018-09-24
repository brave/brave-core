/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Heading, Paragraph } from 'brave-ui/old'
import Image from 'brave-ui/old/v1/image'
import { PushButtonLink } from 'brave-ui/old/v1/pushButton'

// Constants
import { theme } from '../constants/theme'

// Utils
import { getLocale } from '../../../common/locale'

// Assets
const rewardsImage = require('../../../img/welcome/rewards.png')

export default class RewardsScreen extends React.PureComponent {
  render () {
    return (
      <section style={theme.content}>
        <Image customStyle={theme.paymentsImage} src={rewardsImage} />
        <Heading level={1} customStyle={theme.title} text={getLocale('enableBraveRewards')} />
        <Paragraph customStyle={theme.text} text={getLocale('setupBraveRewards')} />
        <PushButtonLink
          color='primary'
          size='large'
          customStyle={theme.mainButton}
          href='chrome://rewards'
        >
          {getLocale('enableRewards')}
        </PushButtonLink>
      </section>
    )
  }
}
