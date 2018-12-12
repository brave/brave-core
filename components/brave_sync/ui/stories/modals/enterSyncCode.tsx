/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import Button from '../../../../src/components/buttonsIndicators/button'
import Modal from '../../../../src/components/popupModals/modal'
import AlertBox from '../../../../src/components/popupModals/alertBox'
import TextAreaClipboard from '../../../../src/components/formControls/textareaClipboard'

// Feature-specific components
import {
  ModalHeader,
  ModalTitle,
  ModalSubTitle,
  ModalContent,
  TwoColumnButtonGrid,
  OneColumnButtonGrid,
  Title,
  SubTitle
} from '../../../../src/features/sync'

// Utils
import { getLocale } from '../page/fakeLocale'

interface Props {
  onClose: () => void
}

interface State {
  passphrase: string
  showAlert: boolean
}

export default class EnterSyncCodeModal extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      passphrase: '',
      showAlert: false
    }
  }

  onEnterPassphrase = (event: React.ChangeEvent<HTMLTextAreaElement>) => {
    this.setState({ passphrase: event.target.value })
  }

  onClickConfirmSyncCode = () => {
    this.setState({ showAlert: true })
  }

  onClickInvalidCode = () => {
    this.setState({ showAlert: false })
  }

  render () {
    const { onClose } = this.props
    const { showAlert } = this.state
    return (
      <Modal id='enterSyncCodeModal' onClose={onClose} size='small'>
      {
        showAlert
        ? (
            <AlertBox okString={getLocale('ok')} onClickOk={this.onClickInvalidCode}>
              <Title level={1}>{getLocale('invalidCode')}</Title>
              <SubTitle>{getLocale('tryAgain')}</SubTitle>
            </AlertBox>
          )
        : null
      }
        <ModalHeader>
          <div>
            <ModalTitle level={1}>{getLocale('enterSyncCode')}</ModalTitle>
            <ModalSubTitle>{getLocale('enterSyncCodeDescription')}</ModalSubTitle>
          </div>
        </ModalHeader>
        <ModalContent>
          <TextAreaClipboard
            copiedString={getLocale('copied')}
            wordCountString={getLocale('wordCount')}
            value={this.state.passphrase}
            onChange={this.onEnterPassphrase}
          />
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
              onClick={this.onClickConfirmSyncCode}
              text={getLocale('confirmCode')}
            />
        </TwoColumnButtonGrid>
      </Modal>
    )
  }
}
