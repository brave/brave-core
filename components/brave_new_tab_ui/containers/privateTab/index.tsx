/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Page, PageWrapper } from '../../components/private'

// Components group
import PrivateTab from './privateTab'
import QwantTab from './qwantTab'
import QwantTorTab from './qwantTorTab'
import TorTab from './torTab'

interface Props {
  actions: any
  newTabData: NewTab.State
}

export default class NewPrivateTabPage extends React.PureComponent<Props, {}> {
  get currentWindowNewTabPage () {
    const { newTabData, actions } = this.props
    if (newTabData.isTor) {
      if (newTabData.isQwant) {
        return <QwantTorTab actions={actions} newTabData={newTabData} />
      }
      return <TorTab actions={actions} newTabData={newTabData} />
    }

    if (newTabData.isQwant) {
      return <QwantTab />
    }

    return <PrivateTab actions={actions} newTabData={newTabData} />
  }

  render () {
    const { isTor, isQwant } = this.props.newTabData
    return (
      <Page isPrivate={!isTor && !isQwant}>
        <PageWrapper>
          {this.currentWindowNewTabPage}
        </PageWrapper>
      </Page>
    )
  }
}
