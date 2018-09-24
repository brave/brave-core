/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Button } from 'brave-ui'

// Component-specific components
import { Grid } from 'brave-ui/features/sync'

// Modals
import NewToSyncModal from './modals/newToSync'
import ExistingSyncCodeModal from './modals/existingSyncCode'

// Utils
import { getLocale } from '../../../common/locale'

interface SyncDisabledContentProps {
  syncData: Sync.State
  actions: any
}

interface SyncDisabledContentState {
  newToSync: boolean
  existingSyncCode: boolean
}

class SyncDisabledContent extends React.PureComponent<SyncDisabledContentProps, SyncDisabledContentState> {
  constructor (props: SyncDisabledContentProps) {
    super(props)
    this.state = {
      newToSync: false,
      existingSyncCode: false
    }
  }

  newToSyncModal = () => {
    this.setState({ newToSync: !this.state.newToSync })
  }

  existingSyncCodeModal = () => {
    this.setState({ existingSyncCode: !this.state.existingSyncCode })
  }

  render () {
    return (
      <Grid columns='auto 1fr'>
        {
          this.state.newToSync
            ? <NewToSyncModal onClose={this.newToSyncModal} />
            : null
        }
        {
          this.state.existingSyncCode
            ? <ExistingSyncCodeModal onClose={this.existingSyncCodeModal} />
            : null
        }
        <div>
          <Button
            level='primary'
            type='accent'
            size='medium'
            onClick={this.newToSyncModal}
            text={getLocale('iAmNewToSync')}
          />
        </div>
        <div>
          <Button
            level='secondary'
            type='accent'
            size='medium'
            onClick={this.existingSyncCodeModal}
            text={getLocale('iHaveAnExistingSyncCode')}
          />
        </div>
      </Grid>
    )
  }
}

export default SyncDisabledContent
