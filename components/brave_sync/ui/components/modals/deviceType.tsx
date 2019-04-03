/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Modal, AlertBox } from 'brave-ui'

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
import ViewSyncCode from './viewSyncCode'
import ScanCode from './scanCode'

// Utils
import { getLocale } from '../../../../common/locale'

// Images
import { SyncDesktopIcon, SyncMobileIcon } from 'brave-ui/features/sync/images'

interface Props {
  syncData: Sync.State
  onClose: () => void
  actions: any
}

interface State {
  viewSyncCode: boolean
  scanCode: boolean
}

export default class DeviceTypeModal extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      viewSyncCode: false,
      scanCode: false
    }
  }

  componentDidMount () {
    // once the screen is rendered, create a sync chain.
    // this allow us to request the qr code and sync words immediately
    const { thisDeviceName } = this.props.syncData
    if (thisDeviceName === '') {
      this.props.actions.onSetupNewToSync('')
    }
  }

  onUserNoticedError = () => {
    this.props.actions.clearSyncSetupError()
    this.props.onClose()
  }

  onClickCancelChildModals = () => {
    this.setState({ scanCode: false, viewSyncCode: false })
  }

  onClickPhoneTabletButton = () => {
    this.setState({ scanCode: true, viewSyncCode: false })
  }

  onClickComputerButton = () => {
    this.setState({ scanCode: false, viewSyncCode: true })
  }

  render () {
    const { onClose, actions, syncData } = this.props
    const { viewSyncCode, scanCode } = this.state

    return (
      <Modal id='deviceTypeModal' displayCloseButton={false} size='small'>
        {
          syncData.error === 'ERR_SYNC_INIT_FAILED'
          ? <AlertBox okString={getLocale('ok')} onClickOk={this.onUserNoticedError}>
              <Title>{getLocale('errorSyncInitFailedTitle')}</Title>
              <SubTitle>{getLocale('errorSyncInitFailedDescription')}</SubTitle>
            </AlertBox>
          : null
        }
        {
          syncData.error === 'ERR_SYNC_REQUIRES_CORRECT_TIME'
          ? <AlertBox okString={getLocale('ok')} onClickOk={this.onUserNoticedError}>
              <Title>{getLocale('errorSyncRequiresCorrectTimeTitle')}</Title>
              <SubTitle>{getLocale('errorSyncRequiresCorrectTimeDescription')}</SubTitle>
            </AlertBox>
          : null
        }
        {
          scanCode
          ? (
            <ScanCode
              syncData={syncData}
              actions={actions}
              onClose={this.onClickCancelChildModals}
              onClickViewSyncCodeInstead={this.onClickComputerButton}
              onCloseDeviceTypeModal={onClose}
            />
          ) : null
        }
        {
          viewSyncCode
            ? (
              <ViewSyncCode
                syncData={syncData}
                actions={actions}
                onClose={this.onClickCancelChildModals}
                onClickScanCodeInstead={this.onClickPhoneTabletButton}
                onCloseDeviceTypeModal={onClose}
              />
            ) : null
        }
        <ModalHeader>
          <div>
            <ModalTitle level={1}>{getLocale('letsSync')}</ModalTitle>
            <ModalSubTitle>{getLocale('chooseDeviceType')}</ModalSubTitle>
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
