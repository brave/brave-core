/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Button, Modal } from 'brave-ui'
import { LoaderIcon } from 'brave-ui/components/icons'

// Feature-specific components
import {
  ModalHeader,
  Title,
  Bold,
  Paragraph,
  ScanGrid,
  ThreeColumnButtonGrid,
  Link
} from 'brave-ui/features/sync'

// Utils
import { getLocale } from '../../../../common/locale'

// Images
import { SyncMobilePicture, QRCode } from 'brave-ui/features/sync/images'

interface Props {
  syncData: Sync.State
  actions: any
  onClose: () => void
  onClickViewSyncCodeInstead: () => void
  onCloseDeviceTypeModal?: () => void
}

interface State {
  newDeviceFound: boolean
}

export default class ScanCodeModal extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      newDeviceFound: false
    }
  }

  componentDidUpdate (prevProps: Readonly<Props>) {
    if (
        prevProps.syncData.devices.length !==
        this.props.syncData.devices.length
    ) {
      this.setState({ newDeviceFound: true })
    }

    const { newDeviceFound } = this.state
    // when a device is found, self-close all modals
    if (newDeviceFound) {
      this.onDismissAllModals()
    }
  }

  onDismissAllModals = () => {
    this.props.onClose()
    if (this.props.onCloseDeviceTypeModal) {
      this.props.onCloseDeviceTypeModal()
    }
  }

  onClickViewCodeWordsInstead = () => {
    this.props.onClickViewSyncCodeInstead()
  }

  render () {
    const { syncData } = this.props
    const { newDeviceFound } = this.state

    return (
      <Modal id='scanCodeModal' displayCloseButton={false} size='small'>
        <ModalHeader>
          <div>
            <Title level={1}>{getLocale('scanThisCode')}</Title>
            <Paragraph>
              {getLocale('scanThisCodeHowToPartial1')} <Bold>{getLocale('scanThisCodeHowToPartial2')}</Bold> {getLocale('scanThisCodeHowToPartial3')}
            </Paragraph>
          </div>
        </ModalHeader>
        <ScanGrid>
          <div><SyncMobilePicture /></div>
          {
            syncData.seedQRImageSource
              ? <QRCode size='normal' src={syncData.seedQRImageSource} />
              : null
          }
        </ScanGrid>
        <ThreeColumnButtonGrid>
          <div>
            <Link onClick={this.onDismissAllModals}>{getLocale('cancel')}</Link>
          </div>
          <div>
            <Button
              level='secondary'
              type='subtle'
              size='medium'
              onClick={this.onClickViewCodeWordsInstead}
              text={getLocale('viewSyncCode')}
            />
          </div>
          <div>
            <Button
              level='primary'
              type='accent'
              size='medium'
              onClick={this.onDismissAllModals}
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
          </div>
        </ThreeColumnButtonGrid>
      </Modal>
    )
  }
}
