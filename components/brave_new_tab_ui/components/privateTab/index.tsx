/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Page, PageWrapper } from 'brave-ui/features/newTab'

// Components group
import PrivateTab from './privateTab'
import QwantTab from './qwantTab'
import QwantTorTab from './qwantTorTab'
import TorTab from './torTab'

interface Props {
  isTor: boolean
  useAlternativePrivateSearchEngine: boolean
  onChangePrivateSearchEngine: (e: React.ChangeEvent<HTMLInputElement>) => void
}

export default class NewPrivateTab extends React.PureComponent<Props, {}> {
  get isQwant () {
    // This is not technically accurately describing whether
    // the browser has been automatically set to a Qwant region.
    // Temporarily we are detecting language here, but we should
    // use the same setting logic as used during first-run.
    // https://github.com/brave/brave-browser/issues/1632
    return navigator.language &&
           navigator.language.startsWith('de') ||
           navigator.language.startsWith('fr')
  }

  get currentWindow () {
    const { isTor, useAlternativePrivateSearchEngine, onChangePrivateSearchEngine } = this.props
    if (isTor) {
      if (this.isQwant) {
        return <QwantTorTab />
      }
      return <TorTab />
    }

    if (this.isQwant) {
      return <QwantTab />
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
      <Page isPrivate={!isTor && !this.isQwant}>
        <PageWrapper>
          {this.currentWindow}
        </PageWrapper>
      </Page>
    )
  }
}
