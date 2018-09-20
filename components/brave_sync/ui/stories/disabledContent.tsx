/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import Button from '../../../src/components/buttonsIndicators/button'

// Component-specific components
import { Grid } from '../../../src/features/sync'

// Modals
import NewToSyncModal from './modals/newToSync'
import ExistingSyncCodeModal from './modals/existingSyncCode'

// Utils
import locale from './page/fakeLocale'

interface SyncDisabledContentState {
  newToSync: boolean
  existingSyncCode: boolean
}

class SyncDisabledContent extends React.PureComponent<{}, SyncDisabledContentState> {
  constructor (props: {}) {
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
            text={locale.iAmNewToSync}
          />
        </div>
        <div>
          <Button
            level='secondary'
            type='accent'
            size='medium'
            onClick={this.existingSyncCodeModal}
            text={locale.iHaveAnExistingSyncCode}
          />
        </div>
      </Grid>
    )
  }
}

export default SyncDisabledContent
