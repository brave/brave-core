/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Button, Modal, AlertBox } from 'brave-ui'

// Feature-specific components
import {
  Title,
  SubTitle,
  ModalHeader,
  ModalTitle,
  DeviceGrid,
  DeviceContainer,
  ModalSubTitle
} from 'brave-ui/features/sync'

// Modals
import AddNewChainNoCameraModal from './addNewChainNoCamera'
import ScanCodeModal from './scanCode'

// Utils
import { getLocale } from '../../../../common/locale'
import { getDefaultDeviceName } from '../../helpers'

// Images
import { SyncAddIcon } from 'brave-ui/features/sync/images'
import { SyncMobileIcon } from 'brave-ui/features/sync/images'
import { SyncDesktopIcon } from 'brave-ui/features/sync/images'

interface Props {
  syncData: Sync.State
  onClose: () => void
  actions: any
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

  onUserNoticedError = () => {
    this.props.actions.resetSyncSetupError()
    this.props.onClose()
  }

  onClickClose = () => {
    this.props.onClose()
  }

  onClickPhoneTabletButton = () => {
    this.setState({ scanCode: !this.state.scanCode })
  }

  onClickComputerButton = () => {
    this.setState({ addNewChainNoCamera: !this.state.addNewChainNoCamera })
  }

  render () {
    const { actions, syncData } = this.props
    const { addNewChainNoCamera, scanCode } = this.state

    if (!syncData) {
      return null
    }

    return (
      <Modal id='deviceTypeModal' onClose={this.onClickClose} size='small'>
        {
          syncData.error === 'ERR_SYNC_NO_INTERNET'
          ? <AlertBox okString={getLocale('ok')} onClickOk={this.onUserNoticedError}>
              <Title>{getLocale('errorNoInternetTitle')}</Title>
              <SubTitle>{getLocale('errorNoInternetDescription')}</SubTitle>
            </AlertBox>
          : null
        }
        {
          syncData.error === 'ERR_SYNC_INIT_FAILED'
          ? <AlertBox okString={getLocale('ok')} onClickOk={this.onUserNoticedError}>
              <Title>{getLocale('errorSyncInitFailedTitle')}</Title>
              <SubTitle>{getLocale('errorSyncInitFailedDescription')}</SubTitle>
            </AlertBox>
          : null
        }
        {
          scanCode
          ? <ScanCodeModal syncData={syncData} actions={actions} onClose={this.onClickPhoneTabletButton} />
          : null
        }
        {
          addNewChainNoCamera
            ? <AddNewChainNoCameraModal syncData={syncData} actions={actions} onClose={this.onClickComputerButton} />
            : null
        }
        <ModalHeader>
          <SyncAddIcon />
          <div>
            <ModalTitle level={1}>{getLocale('letsSync')}<br />“{getDefaultDeviceName()}”.</ModalTitle>
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
