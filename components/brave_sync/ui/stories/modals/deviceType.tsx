/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import Modal from '../../../../src/components/popupModals/modal'

// Feature-specific components
import {
  ModalHeader,
  Title,
  DeviceGrid,
  DeviceContainer,
  Paragraph
} from '../../../../src/features/sync'

// Images
import { SyncMobileIcon, SyncDesktopIcon } from '../../../../src/features/sync/images'

// Modals
import AddNewChainNoCamera from './addNewChainNoCamera'
import ScanCode from './scanCode'

// Utils
import { getLocale } from '../page/fakeLocale'

interface Props {
  onClose: () => void
}

interface State {
  addNewChainNoCamera: boolean
  scanCode: boolean
}

export default class DeviceTypeModal extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      addNewChainNoCamera: false,
      scanCode: false
    }
  }

  onClickPhoneTabletButton = (event: React.MouseEvent<HTMLAnchorElement>) => {
    event.preventDefault()
    this.setState({ scanCode: !this.state.scanCode })
  }

  onClickComputerButton = (event: React.MouseEvent<HTMLAnchorElement>) => {
    event.preventDefault()
    this.setState({ addNewChainNoCamera: !this.state.addNewChainNoCamera })
  }

  render () {
    const { onClose } = this.props
    const { addNewChainNoCamera, scanCode } = this.state
    return (
      <Modal id='deviceTypeModal' onClose={onClose} size='small'>
        {
          scanCode
          ? <ScanCode onClose={this.onClickPhoneTabletButton} />
          : null
        }
        {
          addNewChainNoCamera
            ? <AddNewChainNoCamera onClose={this.onClickComputerButton} />
            : null
        }
        <ModalHeader>
          <div>
            <Title level={1}>{getLocale('letsSync')}</Title>
            <Paragraph>{getLocale('chooseDeviceType')}</Paragraph>
          </div>
        </ModalHeader>
        <DeviceGrid>
          <DeviceContainer onClick={this.onClickPhoneTabletButton}>
            <SyncMobileIcon />
            <Title level={2}>{getLocale('phoneTablet')}</Title>
          </DeviceContainer>
          <DeviceContainer onClick={this.onClickComputerButton}>
            <SyncDesktopIcon />
            <Title level={2}>{getLocale('computer')}</Title>
          </DeviceContainer>
        </DeviceGrid>
      </Modal>
    )
  }
}
