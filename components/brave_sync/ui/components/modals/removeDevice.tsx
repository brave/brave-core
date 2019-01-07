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

// Icons
import { LoaderIcon } from 'brave-ui/components/icons'

// Utils
import { getLocale } from '../../../../common/locale'

interface Props {
  syncData: Sync.State
  onClose: (event?: React.MouseEvent<HTMLButtonElement>) => void
  actions: any
  deviceName: string | undefined
  deviceId: string | undefined
}

interface State {
  willRemoveDevice: boolean
}

export default class RemoveMainDeviceModal extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = { willRemoveDevice: false }
  }

  componentDidUpdate (prevProps: Props) {
    // if devices lengh is different it means that sync
    // computed the device removal. in this case, cancel
    // the loading state and close the modal
    if (
      prevProps.syncData.devices.length !==
      this.props.syncData.devices.length
    ) {
      this.setState({ willRemoveDevice: false })
      this.props.onClose()
    }
  }

  onDismissModal = () => {
    this.props.onClose()
  }

  onClickConfirmRemoveDeviceButton = () => {
    const { syncData, deviceName, deviceId } = this.props
    // if there aren't enough devices, reset sync
    if (syncData.devices.length < 2) {
      this.props.actions.onSyncReset()
      return
    }
    this.props.actions.onRemoveDevice(Number(deviceId), deviceName)
    this.setState({ willRemoveDevice: true })
  }

  render () {
    const { syncData, deviceName, deviceId } = this.props
    const { willRemoveDevice } = this.state

    return (
      <Modal id='removeMainDeviceModal' displayCloseButton={false} size='small'>
        <ModalHeader>
          <ModalTitle level={1}>{getLocale('remove')} “{deviceName}” {getLocale('thisSyncChain')}?</ModalTitle>
        </ModalHeader>
        <ModalContent>
          {
            deviceId === syncData.thisDeviceId
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
                type='subtle'
                size='medium'
                onClick={this.onDismissModal}
                disabled={willRemoveDevice}
                text={getLocale('cancel')}
              />
            </OneColumnButtonGrid>
            <Button
              level='primary'
              type='warn'
              size='medium'
              onClick={this.onClickConfirmRemoveDeviceButton}
              text={getLocale('remove')}
              disabled={willRemoveDevice}
              icon={{
                position: 'after',
                image: willRemoveDevice && <LoaderIcon />
              }}
            />
        </TwoColumnButtonGrid>
      </Modal>
    )
  }
}
