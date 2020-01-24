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
  newDeviceFound: boolean
}

export default class ViewSyncCodeModal extends React.PureComponent<Props, State> {
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
    // when a device is found, self-close this modal
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

  onClickUseCameraInsteadButton = () => {
    this.props.onClickScanCodeInstead()
  }

  render () {
    const { syncData } = this.props
    const { newDeviceFound } = this.state

    return (
      <Modal id='viewSyncCodeModal' displayCloseButton={false} size='small'>
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
            <Link onClick={this.onDismissAllModals}>{getLocale('cancel')}</Link>
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
        </ThreeColumnButtonGrid>
      </Modal>
    )
  }
}
