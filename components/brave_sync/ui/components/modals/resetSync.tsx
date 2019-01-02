/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Button, Modal } from 'brave-ui'

// Feature-specific components
import {
  ModalHeader,
  ModalTitle,
  ModalSubTitle,
  ModalContent,
  TwoColumnButtonGrid,
  OneColumnButtonGrid,
  Paragraph
} from 'brave-ui/features/sync'

// Icons
import { LoaderIcon } from 'brave-ui/components/icons'

// Dialogs
import AreYouSure from '../commonDialogs/areYouSure'

// Utils
import { getLocale } from '../../../../common/locale'

interface Props {
  syncData: Sync.State
  actions: any
  onClose: () => void
}

interface State {
  showAlert: boolean
  willResetSync: boolean
}

export default class ResetSyncModal extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      showAlert: false,
      willResetSync: false
    }
  }

  componentDidUpdate (prevProps: Props) {
    // wait until sync is not configured to proceed
    if (
      prevProps.syncData.isSyncConfigured !==
      this.props.syncData.isSyncConfigured
    ) {
      this.setState({ willResetSync: false })
      this.props.onClose()
    }
  }

  onDismissModal = () => {
    this.props.onClose()
  }

  onClickResetSync = () => {
    this.setState({ showAlert: !this.state.showAlert })
  }

  onConfirmResetSync = () => {
    this.setState({
      showAlert: false,
      willResetSync: true
    })
    this.props.actions.onSyncReset()
  }

  render () {
    const { syncData } = this.props
    const { showAlert, willResetSync } = this.state

    if (!syncData) {
      return null
    }

    return (
      <Modal id='resetSyncModal' displayCloseButton={false} size='small'>
        {
          showAlert &&
          <AreYouSure onClickOk={this.onConfirmResetSync} onClickCancel={this.onClickResetSync} />
        }
        <ModalHeader>
          <div>
            <ModalSubTitle highlight={true}>{getLocale('warning')}</ModalSubTitle>
            <ModalTitle level={1}>{getLocale('removing')} “{syncData.thisDeviceName}” {getLocale('deleteSyncChain')}</ModalTitle>
          </div>
        </ModalHeader>
        <ModalContent>
          <Paragraph>{getLocale('deleteSyncDescription')}</Paragraph>
          <Paragraph>{getLocale('startSyncChainHowTo')}</Paragraph>
        </ModalContent>
        <TwoColumnButtonGrid>
            <OneColumnButtonGrid>
              <Button
                level='secondary'
                type='subtle'
                size='medium'
                onClick={this.onDismissModal}
                text={getLocale('cancel')}
                disabled={willResetSync}
              />
            </OneColumnButtonGrid>
            <Button
              level='primary'
              type='warn'
              size='medium'
              onClick={this.onClickResetSync}
              text={getLocale('remove')}
              disabled={willResetSync}
              icon={{
                position: 'after',
                image: willResetSync && <LoaderIcon />
              }}
            />
          </TwoColumnButtonGrid>
      </Modal>
    )
  }
}
