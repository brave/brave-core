/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Button, Modal, TextAreaClipboard } from 'brave-ui'
import { LoaderIcon } from 'brave-ui/components/icons'

// Feature-specific components
import {
  ModalHeader,
  Title,
  Paragraph,
  ThreeColumnButtonGrid,
  Bold,
  Link
} from 'brave-ui/features/sync'

// Dialogs
import CancelDeviceSyncingDialog from '../commonDialogs/cancelDeviceSyncing'

// Utils
import { getLocale } from '../../../../common/locale'

interface Props {
  syncData: Sync.State
  actions: any
  onClose: () => void
  onClickScanCodeInstead: () => void
  onCloseDeviceTypeModal?: () => void
}

interface State {
  willCancelViewCode: boolean
  newDeviceFound: boolean
}

export default class ViewSyncCodeModal extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      willCancelViewCode: false,
      newDeviceFound: false
    }
  }

  componentDidUpdate (prevProps: Readonly<Props>) {
    if (
        this.props.syncData.devices.length > 1 &&
        prevProps.syncData.devices.length !==
        this.props.syncData.devices.length
    ) {
      this.setState({ newDeviceFound: true })
    }

    const { newDeviceFound } = this.state
    // when a device is found, self-close this modal
    if (newDeviceFound) {
      this.dismissAllModals()
    }
  }

  dismissAllModals = () => {
    this.props.onClose()
    if (this.props.onCloseDeviceTypeModal) {
      this.props.onCloseDeviceTypeModal()
    }
  }

  onClickUseCameraInsteadButton = () => {
    this.props.onClickScanCodeInstead()
  }

  onDismissModal = () => {
    const { devices, isSyncConfigured } = this.props.syncData
    // if user is still trying to build a sync chain,
    // open the confirmation modal. otherwise close it
    isSyncConfigured && devices.length < 2
      ? this.setState({ willCancelViewCode: true })
      : this.dismissAllModals()
  }

  onDismissDialog = () => {
    this.setState({ willCancelViewCode: false })
  }

  onConfirmDismissModal = () => {
    const { devices, isSyncConfigured } = this.props.syncData
    // sync is enabled when at least 2 devices are in the chain.
    // this modal works both with sync enabled and disabled states.
    // in case user opens it in the enabled content screen,
    // check there are 2 devices in chain before reset
    if (isSyncConfigured && devices.length < 2) {
      this.props.actions.onSyncReset()
      this.dismissAllModals()
    }
    this.setState({ willCancelViewCode: false })
    this.props.onClose()
  }

  render () {
    const { syncData } = this.props
    const { willCancelViewCode, newDeviceFound } = this.state

    return (
      <Modal id='viewSyncCodeModal' displayCloseButton={false} size='small'>
        {
          willCancelViewCode
          ? <CancelDeviceSyncingDialog onClickCancel={this.onDismissDialog} onClickOk={this.onConfirmDismissModal} />
          : null
        }
        <ModalHeader>
          <div>
            <Title level={1}>{getLocale('chainCode')}</Title>
            <Paragraph>
              {getLocale('chainCodeDescriptionPartial1')} <Bold>{getLocale('chainCodeDescriptionPartial2')}</Bold> {getLocale('chainCodeDescriptionPartial3')}
            </Paragraph>
          </div>
        </ModalHeader>
          {
            syncData.syncWords
            ? (
              <TextAreaClipboard
                copiedString={getLocale('copied')}
                wordCountString={getLocale('wordCount')}
                readOnly={true}
                defaultValue={syncData.syncWords}
              />
            )
            : null
          }
        <ThreeColumnButtonGrid>
          <div>
            <Link onClick={this.onDismissModal}>{getLocale('cancel')}</Link>
          </div>
          <div>
            <Button
              level='secondary'
              type='subtle'
              size='medium'
              onClick={this.onClickUseCameraInsteadButton}
              text={getLocale('qrCode')}
            />
          </div>
          <Button
            level='primary'
            type='accent'
            size='medium'
            onClick={this.onDismissModal}
            disabled={newDeviceFound === false}
            text={
              newDeviceFound === false
              ? getLocale('lookingForDevice')
              : getLocale('ok')
            }
            icon={{
              position: 'before',
              image: newDeviceFound === false
                ? <LoaderIcon />
                : null
            }}
          />
        </ThreeColumnButtonGrid>
      </Modal>
    )
  }
}
