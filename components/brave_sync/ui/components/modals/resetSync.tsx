/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Button, Modal } from 'brave-ui'

// Feature-specific components
import {
  Title,
  ListOrdered,
  ListBullet,
  Grid,
  FlexColumn
} from 'brave-ui/features/sync'

// Assets
import { getLocale } from '../../../../common/locale'

interface ResetSyncModalProps {
  onClose: () => void
}

interface ResetSyncModalState {
  showAreYouSureAlert: boolean
}

class ResetSyncModal extends React.PureComponent<ResetSyncModalProps, ResetSyncModalState> {
  constructor (props: ResetSyncModalProps) {
    super(props)
    this.state = { showAreYouSureAlert: false }
  }

  areYouSureAlert = () => {
    if (window.confirm(getLocale('areYouSure'))) {
      this.props.onClose()
      // fire sync reset
    }
  }

  render () {
    const { onClose } = this.props
    return (
      <Modal id='showIAmResetSyncModal' onClose={onClose} size='small'>
        <Title level={1}>{getLocale('resetSync')}</Title>
        <ListOrdered>
          <ListBullet>{getLocale('resetSyncFirstBullet')}</ListBullet>
          <ListBullet>{getLocale('resetSyncSecondBullet')}</ListBullet>
          <ListBullet>{getLocale('resetSyncThirdBullet')}</ListBullet>
        </ListOrdered>
        <Grid columns='auto 0fr'>
          <FlexColumn content='flex-end'>
            <Button
              level='secondary'
              type='accent'
              size='medium'
              onClick={onClose}
              text={getLocale('cancel')}
            />
          </FlexColumn>
          <Button
            level='primary'
            type='accent'
            size='medium'
            onClick={this.areYouSureAlert}
            text={getLocale('resetSync')}
          />
        </Grid>
      </Modal>
    )
  }
}

export default ResetSyncModal
