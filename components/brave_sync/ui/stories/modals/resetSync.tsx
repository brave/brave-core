/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import Button from '../../../../src/components/buttonsIndicators/button'
import Modal from '../../../../src/components/popupModals/modal'

// Feature-specific components
import {
  Title,
  ListOrdered,
  ListBullet,
  Grid,
  FlexColumn
} from '../../../../src/features/sync'

// Assets
import locale from '../page/fakeLocale'

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
    if (window.confirm(locale.areYouSure)) {
      this.props.onClose()
      // fire sync reset
    }
  }

  render () {
    const { onClose } = this.props
    return (
      <Modal id='showIAmResetSyncModal' onClose={onClose} size='small'>
        <Title level={1}>{locale.resetSync}</Title>
        <ListOrdered>
          <ListBullet>{locale.resetSyncFirstBullet}</ListBullet>
          <ListBullet>{locale.resetSyncSecondBullet}</ListBullet>
          <ListBullet>{locale.resetSyncThirdBullet}</ListBullet>
        </ListOrdered>
        <Grid columns='auto 0fr'>
          <FlexColumn content='flex-end'>
            <Button
              level='secondary'
              type='accent'
              size='medium'
              onClick={onClose}
              text={locale.cancel}
            />
          </FlexColumn>
          <Button
            level='primary'
            type='accent'
            size='medium'
            onClick={this.areYouSureAlert}
            text={locale.resetSync}
          />
        </Grid>
      </Modal>
    )
  }
}

export default ResetSyncModal
