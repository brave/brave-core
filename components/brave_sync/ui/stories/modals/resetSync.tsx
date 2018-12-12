/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import Button from '../../../../src/components/buttonsIndicators/button'
import Modal from '../../../../src/components/popupModals/modal'
import AlertBox from '../../../../src/components/popupModals/alertBox'

// Feature-specific components
import {
  ModalHeader,
  ModalTitle,
  ModalSubTitle,
  ModalContent,
  TwoColumnButtonGrid,
  OneColumnButtonGrid,
  Title,
  Paragraph
} from '../../../../src/features/sync'

// Utils
import { getLocale } from '../page/fakeLocale'

interface Props {
  mainDeviceName: string
  onClose: () => void
}

interface State {
  showAlert: boolean
}

export default class ResetSyncModal extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = { showAlert: false }
  }

  onSetupSync = () => {
    this.setState({ showAlert: !this.state.showAlert })
  }

  onResetSync = () => {
    this.props.onClose()
  }

  render () {
    const { onClose, mainDeviceName } = this.props
    const { showAlert } = this.state
    return (
      <Modal id='resetSyncModal' onClose={onClose} size='small'>
        {
          showAlert
          ? (
              <AlertBox
                okString={getLocale('ok')}
                onClickOk={this.onResetSync}
                cancelString={getLocale('cancel')}
                onClickCancel={onClose}
              >
                <Title level={1}>{getLocale('areYouSure')}</Title>
              </AlertBox>
            )
          : null
        }
        <ModalHeader>
          <div>
            <ModalSubTitle highlight={true}>{getLocale('warning')}</ModalSubTitle>
            <ModalTitle level={1}>{getLocale('removing')} “{mainDeviceName}” {getLocale('deleteSyncChain')}</ModalTitle>
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
                onClick={onClose}
                text={getLocale('cancel')}
              />
            </OneColumnButtonGrid>
            <Button
              level='primary'
              type='warn'
              size='medium'
              onClick={this.onSetupSync}
              text={getLocale('remove')}
            />
          </TwoColumnButtonGrid>
      </Modal>
    )
  }
}
