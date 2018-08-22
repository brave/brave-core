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
  StyledDoneWrapper,
  StyledStatus,
  GroupedButton,
  ActionButton
} from './style'
import { TextArea, Tabs, Modal, Button } from '../../../components'
import { getLocale } from '../../../helpers'
import Alert from '../alert'
import ControlWrapper from '../../../components/formControls/controlWrapper'

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
  onFileUpload = (inputFile: React.ChangeEvent<HTMLInputElement>) => {
    const input: HTMLInputElement = inputFile.target
    const self = this

    if (!input.files) {
      return
    }

    const reader = new FileReader()
    reader.onload = function () {
      if (reader.result) {
        self.onRestore((reader.result.toString() || '').trim())
      } else {
        self.onRestore('')
      }
    }

    try {
      reader.readAsText(input.files[0])
    } catch (e) {
      self.onRestore('')
    }
  }

  setRecoveryKey = (event: React.ChangeEvent<HTMLTextAreaElement>) => {
    this.setState({
      recoveryKey: event.target.value
    })
  }

  onRestore = (key?: string) => {
    key = typeof key === 'string' ? key : this.state.recoveryKey
    this.setState({
      recoveryKey: ''
    })
    this.props.onRestore(key)
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
      error
    } = this.props

    return (
      <Modal id={id} onClose={onClose} customStyle={{ maxWidth: '666px' }}>
        <StyledWrapper>
          <Tabs activeTabId={activeTabId} onChange={onTabChange}>
          <div id={`${id}-backup`} data-key={'backup'} data-title={getLocale('rewardsBackupText1')}>
            <StyledContent>
              {getLocale('rewardsBackupText2')}
            </StyledContent>
            <ControlWrapper text={getLocale('recoveryKeys')}>
              <TextArea
                value={backupKey}
                disabled={true}
              />
            </ControlWrapper>
            <StyleButtonWrapper>
              <GroupedButton
                text={getLocale('copy')}
                level={'secondary'}
                size={'small'}
                type={'subtle'}
                onClick={onCopy.bind(this, backupKey)}
              />
              <GroupedButton
                text={getLocale('print')}
                level={'secondary'}
                size={'small'}
                type={'subtle'}
                onClick={onPrint.bind(this, backupKey)}
              />
              <GroupedButton
                text={getLocale('saveAsFile')}
                level={'secondary'}
                size={'small'}
                type={'subtle'}
                onClick={onSaveFile.bind(this, backupKey)}
              />
            </StyleButtonWrapper>
            <StyledDoneWrapper>
              <Button
                text={getLocale('done')}
                level={'secondary'}
                size={'medium'}
                onClick={onClose}
              />
            </StyledDoneWrapper>
          </div>
          <div id={`${id}-restore`} data-key={'restore'} data-title={getLocale('rewardsRestoreText1')}>
            <StyledContent>
              {getLocale('rewardsRestoreText2')}
            </StyledContent>
            <StyledStatus error={error}>
              {
                error
                ? <Alert type={'error'} color={true} bg={true}>
                    {error}
                </Alert>
                : null
              }
            </StyledStatus>
            <ControlWrapper
              text={
                <>
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
                </>
              }
            >
              <TextArea
                value={this.state.recoveryKey}
                onChange={this.setRecoveryKey}
              />
            </ControlWrapper>
            <StyledActionsWrapper>
              <ActionButton
                level={'secondary'}
                text={getLocale('cancel')}
                size={'medium'}
                type={'accent'}
                onClick={onClose}
              />
              <ActionButton
                level={'primary'}
                type={'accent'}
                text={getLocale('restore')}
                size={'medium'}
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
