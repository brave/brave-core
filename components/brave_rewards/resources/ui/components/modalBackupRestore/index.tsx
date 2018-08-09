/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledWrapper,
  StyledContent,
  StyledImport,
  StyleButtonWrapper,
  StyledActionsWrapper,
  StyledDoneWrapper
} from './style'
import Tabs from '../../../components/layout/tabs/index'
import TextArea from '../../../components/formControls/textarea/index'
import Modal from '../../../components/popupModals/modal/index'
import ButtonPrimary from '../../../components/buttonsIndicators/buttonPrimary/index'
import ButtonSecondary from '../../../components/buttonsIndicators/buttonSecondary/index'
import { getLocale } from '../../../helpers'
import Alert from '../alert'

export type TabsType = 'backup' | 'restore'

export interface Props {
  backupKey: string
  activeTabId: TabsType
  onTabChange: (tab: TabsType) => void
  onClose: () => void
  onCopy: (key: string) => void
  onPrint: (key: string) => void
  onSaveFile: (key: string) => void
  onRestore: (key: string) => void
  error?: React.ReactNode
  success?: React.ReactNode
  id?: string
}

interface State {
  recoveryKey: string
}

/*
  TODO
  - add error flow
 */
export default class ModalBackupRestore extends React.PureComponent<Props, State> {

  constructor (props: Props) {
    super(props)
    this.state = {
      recoveryKey: ''
    }
  }

  componentDidUpdate (prevProps: Props) {
    if (this.props.success !== prevProps.success) {
      this.setState({
        recoveryKey: ''
      })
    }
  }

  onFileUpload = (inputFile: React.ChangeEvent<HTMLInputElement>) => {
    const input: HTMLInputElement = inputFile.target
    const self = this

    if (!input.files) {
      return
    }

    const reader = new FileReader()
    reader.onload = function () {
      self.props.onRestore((reader.result || '').trim())
    }

    try {
      reader.readAsText(input.files[0])
    } catch (e) {
      self.props.onRestore('')
    }
  }

  setRecoveryKey = (event: React.ChangeEvent<HTMLTextAreaElement>) => {
    this.setState({
      recoveryKey: event.target.value
    })
  }

  onRestore = () => {
    this.props.onRestore(this.state.recoveryKey)
  }

  render () {
    const {
      id,
      backupKey,
      activeTabId,
      onClose,
      onTabChange,
      onCopy,
      onPrint,
      onSaveFile,
      error,
      success
    } = this.props

    return (
      <Modal id={id} onClose={onClose} theme={{ maxWidth: '666px' }}>
        <StyledWrapper>
          <Tabs activeTabId={activeTabId} onChange={onTabChange}>
          <div id={`${id}-backup`} data-key={'backup'} data-title={getLocale('rewardsBackupText1')}>
            <StyledContent>
              {getLocale('rewardsBackupText2')}
            </StyledContent>
            <TextArea
              title={getLocale('recoveryKeys')}
              theme={{ maxWidth: '100%', minHeight: '140px' }}
              value={backupKey}
              disabled={true}
            />
            <StyleButtonWrapper>
              <ButtonSecondary
                text={getLocale('copy')}
                size={'small'}
                color={'subtle'}
                onClick={onCopy.bind(this, backupKey)}
              />
              <ButtonSecondary
                text={getLocale('print')}
                size={'small'}
                color={'subtle'}
                onClick={onPrint.bind(this, backupKey)}
              />
              <ButtonSecondary
                text={getLocale('saveAsFile')}
                size={'small'}
                color={'subtle'}
                onClick={onSaveFile.bind(this, backupKey)}
              />
            </StyleButtonWrapper>
            <StyledDoneWrapper>
              <ButtonPrimary
                text={getLocale('done')}
                size={'medium'}
                color={'brand'}
                onClick={onClose}
              />
            </StyledDoneWrapper>
          </div>
          <div id={`${id}-restore`} data-key={'restore'} data-title={getLocale('rewardsRestoreText1')}>
            <StyledContent error={error} success={success}>
              {getLocale('rewardsRestoreText2')}
            </StyledContent>
            {
              error
              ? <Alert type={'error'}>
                  {error}
              </Alert>
              : null
            }
            {
              success
              ? <Alert type={'success'}>
                {success}
              </Alert>
              : null
            }
            <TextArea
              title={<>
                {getLocale('rewardsRestoreText3')} <StyledImport
                  htmlFor={'recoverFile'}
                >
                  {getLocale('import')}
                </StyledImport>
                <input
                  type='file'
                  id='recoverFile'
                  name='recoverFile'
                  style={{ display: 'none' }}
                  onChange={this.onFileUpload}
                />
              </>}
              theme={{ maxWidth: '100%', minHeight: '140px' }}
              value={this.state.recoveryKey}
              onChange={this.setRecoveryKey}
            />
            <StyledActionsWrapper>
              <ButtonSecondary
                text={getLocale('cancel')}
                size={'medium'}
                color={'brand'}
                onClick={onClose}
              />
              <ButtonPrimary
                text={getLocale('restore')}
                size={'medium'}
                color={'brand'}
                onClick={this.onRestore}
              />
            </StyledActionsWrapper>
          </div>
        </Tabs>
        </StyledWrapper>
      </Modal>
    )
  }
}
