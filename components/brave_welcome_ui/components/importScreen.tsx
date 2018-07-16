/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from '../../common/locale'
import Image from 'brave-ui/v1/image'
import { PushButton } from 'brave-ui/v1/pushButton'
import { Heading, Paragraph } from 'brave-ui'
import { theme } from '../theme'

const importImage = require('../../img/welcome/import.png')

interface Props {
  onImportNowClicked: () => void
}

export default class ImportScreen extends React.PureComponent<Props, {}> {
  onClickImportNow = () => {
    this.props.onImportNowClicked()
  }

  render () {
    return (
      <section style={theme.content}>
        <Image theme={theme.importImage} src={importImage} />
        <Heading level={1} theme={theme.title} text={getLocale('importFromAnotherBrowser')} />
        <Paragraph theme={theme.text} text={getLocale('setupImport')} />
        <PushButton
          color='primary'
          size='large'
          theme={theme.mainButton}
          onClick={this.onClickImportNow}
        >
          {getLocale('importNow')}
        </PushButton>
      </section>
    )
  }
}
