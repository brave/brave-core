/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Heading, Paragraph } from 'brave-ui'
import Image from 'brave-ui/v1/image'
import { PushButtonLink } from 'brave-ui/v1/pushButton'

// Constants
import { theme } from '../constants/theme'

// Utils
import { getLocale } from '../../common/locale'

// Assets
const rewardsImage = require('../../img/welcome/rewards.png')

export default class RewardsScreen extends React.PureComponent {
  render () {
    return (
      <section style={theme.content}>
        <Image theme={theme.paymentsImage} src={rewardsImage} />
        <Heading level={1} theme={theme.title} text={getLocale('enableBraveRewards')} />
        <Paragraph theme={theme.text} text={getLocale('setupBraveRewards')} />
        <PushButtonLink
          color='primary'
          size='large'
          theme={theme.mainButton}
          href='chrome://rewards'
        >
          {getLocale('enableRewards')}
        </PushButtonLink>
      </section>
    )
  }
}
