/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Component groups
import DisabledContent from '../disabledContent'
import EnabledContent from '../enabledContent'

interface SyncPageProps {
  // Note: this is for demonstration purposes and
  // should not be included in production
  disabled?: boolean
}

interface SyncPageState {
  enabledContent: boolean
  syncResetModalOpen: boolean
}

class SyncPage extends React.PureComponent<SyncPageProps, SyncPageState> {
  render () {
    return this.props.disabled
      ? <DisabledContent />
      : <EnabledContent />
  }
}

export default SyncPage
