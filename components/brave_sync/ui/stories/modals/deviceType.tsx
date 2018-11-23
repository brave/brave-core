/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import Button from '../../../../src/components/buttonsIndicators/button'
import Modal from '../../../../src/components/popupModals/modal'

// Feature-specific components
import {
  ModalHeader,
  ModalTitle,
  DeviceGrid,
  DeviceContainer,
  ModalSubTitle
} from '../../../../src/features/sync'

// Images
import { SyncAddIcon, SyncMobileIcon, SyncDesktopIcon } from '../../../../src/features/sync/images'

// Modals
import AddNewChainNoCamera from './addNewChainNoCamera'
import ScanCode from './scanCode'

// Utils
import { getLocale } from '../page/fakeLocale'

interface Props {
  mainDeviceName: string
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

  onClickPhoneTabletButton = () => {
    this.setState({ scanCode: !this.state.scanCode })
  }

  onClickComputerButton = () => {
    this.setState({ addNewChainNoCamera: !this.state.addNewChainNoCamera })
  }

  render () {
    const { onClose, mainDeviceName } = this.props
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
          <SyncAddIcon />
          <div>
            <ModalTitle level={1}>{getLocale('letsSync')}<br />“{mainDeviceName}”.</ModalTitle>
            <ModalSubTitle>{getLocale('chooseDeviceType')}</ModalSubTitle>
          </div>
        </ModalHeader>
        <DeviceGrid>
          <DeviceContainer>
            <SyncMobileIcon />
            <Button
              level='primary'
              type='accent'
              size='medium'
              onClick={this.onClickPhoneTabletButton}
              text={getLocale('phoneTablet')}
            />
          </DeviceContainer>
          <DeviceContainer>
            <SyncDesktopIcon />
            <Button
              level='primary'
              type='accent'
              size='medium'
              onClick={this.onClickComputerButton}
              text={getLocale('computer')}
            />
          </DeviceContainer>
        </DeviceGrid>
      </Modal>
    )
  }
}
