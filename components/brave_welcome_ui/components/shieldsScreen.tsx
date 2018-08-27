/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Image from 'brave-ui/old/v1/image'
import { Heading, Paragraph } from 'brave-ui/old'

// Constants
import { theme } from '../constants/theme'

// Utils
import { getLocale } from '../../common/locale'

// Assets
const shieldsImage = require('../../img/welcome/shields.png')

export default class ShieldsScreen extends React.PureComponent {
  render () {
    return (
      <section style={theme.content}>
        <Image customStyle={theme.shieldsImage} src={shieldsImage} />
        <Heading level={1} customStyle={theme.title} text={getLocale('manageShields')} />
        <Paragraph customStyle={theme.text} text={getLocale('adjustProtectionLevel')} />
      </section>
    )
  }
}
