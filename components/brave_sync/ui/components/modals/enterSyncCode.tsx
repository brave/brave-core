/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { AlertBox, Button, Modal, TextAreaClipboard } from 'brave-ui'

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
} from 'brave-ui/features/sync'

// Utils
import { getLocale } from '../../../../common/locale'

interface Props {
  syncData: Sync.State
  actions: any
  onClose: () => void
}
 interface State {
  passphrase: string
}
 export default class EnterSyncCodeModal extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      passphrase: ''
    }
  }

  onUserNoticedError = () => {
    this.props.actions.resetSyncSetupError()
  }

  onEnterPassphrase = (event: React.ChangeEvent<HTMLTextAreaElement>) => {
    this.setState({ passphrase: event.target.value })
  }

  onClickConfirmSyncCode = () => {
    const { error, thisDeviceName } = this.props.syncData
    if (thisDeviceName !== '' || error) {
      return
    }
    const { passphrase } = this.state
    this.props.actions.onSetupSyncHaveCode(passphrase, '')
  }

  render () {
    const { onClose, syncData } = this.props
    return (
      <Modal id='enterSyncCodeModal' onClose={onClose} size='small'>
        {
           syncData.error === 'ERR_SYNC_WRONG_WORDS'
           ? <AlertBox okString={getLocale('ok')} onClickOk={this.onUserNoticedError}>
               <Title>{getLocale('errorWrongCodeTitle')}</Title>
               <SubTitle>{getLocale('errorWrongCodeDescription')}</SubTitle>
           </AlertBox>
           : null
        }
        {
           syncData.error === 'ERR_SYNC_MISSING_WORDS'
           ? <AlertBox okString={getLocale('ok')} onClickOk={this.onUserNoticedError}>
               <Title>{getLocale('errorMissingCodeTitle')}</Title>
           </AlertBox>
           : null
        }
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
        <ModalHeader>
          <div>
            <ModalTitle level={1}>{getLocale('enterSyncCode')}</ModalTitle>
            <ModalSubTitle>{getLocale('enterSyncCodeDescription')}</ModalSubTitle>
          </div>
        </ModalHeader>
        <ModalContent>
          <TextAreaClipboard
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
