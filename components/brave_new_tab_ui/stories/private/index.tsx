/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Page, PageWrapper } from '../../../../src/features/newTab/private'

// Components group
import PrivateWindow from './privateWindow'
import QwantWindow from './qwantWindow'
import TorWindow from './torWindow'
import QwantTor from './qwantWindowWithTor'

// Assets
import '../../../assets/fonts/muli.css'
import '../../../assets/fonts/poppins.css'

interface Props {
  isTor: boolean
  isQwant: boolean
}

export default class NewPrivateTab extends React.PureComponent<Props, {}> {
  get currentWindow () {
    const { isTor, isQwant } = this.props
    return isQwant && isTor
      ? <QwantTor />
      : isQwant ? <QwantWindow />
      : isTor ? <TorWindow />
      : <PrivateWindow />
  }
  render () {
    const { isTor, isQwant } = this.props
    return (
      <Page isPrivate={!isTor && !isQwant}>
        <PageWrapper>
          {this.currentWindow}
        </PageWrapper>
      </Page>
    )
  }
}
