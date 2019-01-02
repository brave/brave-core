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

// Icons
import { LoaderIcon } from 'brave-ui/components/icons'

// Utils
import { getLocale } from '../../../../common/locale'

interface Props {
  syncData: Sync.State
  actions: any
  onClose: () => void
}
interface State {
  passphrase: string
  willCreateNewSyncChainFromCode: boolean
}

export default class EnterSyncCodeModal extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      passphrase: '',
      willCreateNewSyncChainFromCode: false
    }
  }

  componentDidUpdate (prevProps: Props) {
    // when component updates with a different config, disable the
    // loading state and unfreeze the modal. at this point the component
    // auto refresh and lead the user to the enabledContent view
    if (
      this.props.syncData.error !== undefined ||
      (
        this.state.willCreateNewSyncChainFromCode &&
        prevProps.syncData.isSyncConfigured !==
        this.props.syncData.isSyncConfigured &&
        this.props.syncData.devices.length > 1
      )
    ) {
      this.setState({ willCreateNewSyncChainFromCode: false })
      this.props.onClose()
    }
  }

  onUserNoticedError = () => {
    this.props.actions.clearSyncSetupError()
  }

  onEnterPassphrase = (event: React.ChangeEvent<HTMLTextAreaElement>) => {
    this.setState({ passphrase: event.target.value })
  }

  onDismissModal = () => {
    this.props.onClose()
  }

  onClickConfirmSyncCode = () => {
    const { error, thisDeviceName } = this.props.syncData
    if (thisDeviceName !== '' || error) {
      return
    }
    const { passphrase } = this.state
    this.setState({ willCreateNewSyncChainFromCode: true })
    this.props.actions.onSetupSyncHaveCode(passphrase, '')
  }

  render () {
    const { syncData } = this.props
    const { willCreateNewSyncChainFromCode } = this.state

    return (
      <Modal id='enterSyncCodeModal' displayCloseButton={false} size='small'>
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
              disabled={willCreateNewSyncChainFromCode}
              onClick={this.onDismissModal}
              text={getLocale('cancel')}
            />
          </OneColumnButtonGrid>
          <Button
            level='primary'
            type='accent'
            size='medium'
            onClick={this.onClickConfirmSyncCode}
            text={getLocale('confirmCode')}
            disabled={willCreateNewSyncChainFromCode}
            icon={{
              position: 'after',
              image: willCreateNewSyncChainFromCode && <LoaderIcon />
            }}
          />
        </TwoColumnButtonGrid>
      </Modal>
    )
  }
}
