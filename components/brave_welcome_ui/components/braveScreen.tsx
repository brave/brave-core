/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Image from 'brave-ui/old/v1/image'
import { PushButton } from 'brave-ui/old/v1/pushButton'
import { Heading, Paragraph } from 'brave-ui/old'

// Constants
import { theme } from '../constants/theme'

// Utils
import { getLocale } from '../../common/locale'

// Assets
const braveLogo = require('../../img/welcome/brave_logo.png')

interface Props {
  onGoToFirstSlide: () => void
}

export default class BraveScreen extends React.PureComponent<Props, {}> {
  onClickLetsGo = () => {
    this.props.onGoToFirstSlide()
  }

  render () {
    return (
      <section style={theme.content}>
        <Image customStyle={theme.braveLogo} src={braveLogo} />
        <Heading level={1} customStyle={theme.title} text={getLocale('welcome')} />
        <Paragraph customStyle={theme.text} text={getLocale('whatIsBrave')} />
        <PushButton
          color='primary'
          size='large'
          customStyle={theme.mainButton}
          onClick={this.onClickLetsGo}
        >
          {getLocale('letsGo')}
        </PushButton>
      </section>
    )
  }
}
