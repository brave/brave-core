/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Page, PageWrapper } from 'brave-ui/features/newTab'

// Components group
import PrivateTab from './privateTab'
import TorTab from './torTab'

interface Props {
  isTor: boolean
  useAlternativePrivateSearchEngine: boolean
  onChangePrivateSearchEngine: (e: React.ChangeEvent<HTMLInputElement>) => void
}

export default class NewPrivateTab extends React.PureComponent<Props, {}> {
  get currentWindow () {
    const { isTor, useAlternativePrivateSearchEngine, onChangePrivateSearchEngine } = this.props
    if (isTor) {
      return <TorTab />
    }

    return (
      <PrivateTab
        useAlternativePrivateSearchEngine={useAlternativePrivateSearchEngine}
        onChangePrivateSearchEngine={onChangePrivateSearchEngine}
      />
    )
  }

  render () {
    const { isTor } = this.props
    return (
      <Page isPrivate={!isTor}>
        <PageWrapper>
          {this.currentWindow}
        </PageWrapper>
      </Page>
    )
  }
}
