/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Button, Modal } from 'brave-ui'

// Feature-specific components
import {
  Paragraph,
  ModalHeader,
  ModalTitle,
  ModalContent,
  TwoColumnButtonGrid,
  OneColumnButtonGrid
} from 'brave-ui/features/sync'

// Utils
import { getLocale } from '../../../../common/locale'

// Images
import { SyncRemoveIcon } from 'brave-ui/features/sync/images'

interface Props {
  onClose: (event?: React.MouseEvent<HTMLButtonElement>) => void
  actions: any
  deviceName: string | undefined
  deviceId: number | undefined
}

export default class RemoveMainDeviceModal extends React.PureComponent<Props, {}> {
  onClickConfirmRemoveDeviceButton = () => {
    const { deviceName, deviceId } = this.props
    this.props.actions.onRemoveDevice(Number(deviceId), deviceName)
    this.props.onClose()
  }

  render () {
    const { onClose, deviceName, deviceId } = this.props

    return (
      <Modal id='removeMainDeviceModal' onClose={onClose} size='small'>
        <ModalHeader>
          <SyncRemoveIcon />
          <ModalTitle level={1}>{getLocale('remove')} “{deviceName}” {getLocale('thisSyncChain')}?</ModalTitle>
        </ModalHeader>
        <ModalContent>
          {
            // zero is always the main device
            deviceId === 0
            ? <div>
                <Paragraph>{getLocale('thisDeviceRemovalDescription')}</Paragraph>
                <Paragraph>{getLocale('joinSyncChain')}</Paragraph>
              </div>
            : <Paragraph>{getLocale('otherDeviceRemovalDescription')}</Paragraph>
          }
        </ModalContent>
        <TwoColumnButtonGrid>
            <OneColumnButtonGrid>
              <Button
                level='secondary'
                type='accent'
                size='medium'
                onClick={onClose}
                text={getLocale('cancel')}
              />
            </OneColumnButtonGrid>
            <Button
              level='primary'
              type='accent'
              size='medium'
              onClick={this.onClickConfirmRemoveDeviceButton}
              text={getLocale('remove')}
            />
        </TwoColumnButtonGrid>
      </Modal>
    )
  }
}
