/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledContent,
  StyledImport,
  StyledButtonWrapper,
  StyledActionsWrapper,
  StyledDoneWrapper,
  StyledLink,
  StyledStatus,
  GroupedButton,
  ActionButton,
  StyledTitle,
  StyledTitleWrapper,
  StyledSafe,
  StyledControlWrapper,
  StyledText,
  StyledTextWrapper
} from './style'
import { TextArea, Modal, Button } from 'brave-ui/components'
import { getLocale } from 'brave-ui/helpers'
import { Alert, Tab } from '../'
import ControlWrapper from 'brave-ui/components/formControls/controlWrapper'

export interface Props {
  backupKey: string
  activeTabId: number
  showBackupNotice: boolean
  walletProvider: string
  onTabChange: (newTabId: number) => void
  onClose: () => void
  onCopy?: (key: string) => void
  onPrint?: (key: string) => void
  onSaveFile?: (key: string) => void
  onRestore: (key: string) => void
  onVerify?: () => void
  error?: React.ReactNode
  id?: string
  testId?: string
  funds?: string
  onReset: () => void
  internalFunds: number
}

interface State {
  recoveryKey: string
  errorShown: boolean
}

/*
  TODO
  - add error flow
 */
export default class ModalBackupRestore extends React.PureComponent<Props, State> {

  constructor (props: Props) {
    super(props)
    this.state = {
      recoveryKey: '',
      errorShown: false
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
      errorShown: false,
      recoveryKey: event.target.value
    })
  }

  onRestore = (key?: string) => {
    key = typeof key === 'string' ? key : this.state.recoveryKey
    this.props.onRestore(key)
  }

  componentWillReceiveProps (nextProps: Props) {
    if (nextProps.error) {
      this.setState({
        errorShown: true
      })
    }
  }

  getBackupLegacy = () => {
    const {
      backupKey,
      onClose,
      onCopy,
      onPrint,
      onSaveFile,
      onVerify,
      walletProvider
    } = this.props

    return (
      <>
        <StyledContent>
          <StyledSafe>
            {getLocale('rewardsBackupText1')}
          </StyledSafe>
          {getLocale('rewardsBackupText2')}
        </StyledContent>
        <ControlWrapper text={getLocale('recoveryKeys')}>
          <TextArea
            id={'backup-recovery-key'}
            value={backupKey}
            disabled={true}
          />
        </ControlWrapper>
        <StyledButtonWrapper>
          {
            onCopy
            ? <GroupedButton
              text={getLocale('copy')}
              level={'secondary'}
              size={'small'}
              type={'subtle'}
              onClick={onCopy.bind(this, backupKey)}
            />
            : null
          }
          {
            onPrint
            ? <GroupedButton
              text={getLocale('print')}
              level={'secondary'}
              size={'small'}
              type={'subtle'}
              onClick={onPrint.bind(this, backupKey)}
            />
            : null
          }
          {
            onSaveFile
            ? <GroupedButton
              text={getLocale('saveAsFile')}
              level={'secondary'}
              size={'small'}
              type={'subtle'}
              onClick={onSaveFile.bind(this, backupKey)}
            />
            : null
          }
        </StyledButtonWrapper>
        <StyledContent>
          <StyledSafe>
            {getLocale('rewardsBackupText3')}
          </StyledSafe>
          {getLocale('rewardsBackupText4')}
        </StyledContent>
        <StyledContent>
            {getLocale('rewardsBackupText5')}
          <StyledLink onClick={onVerify}>
            {getLocale('rewardsBackupText6').replace('$1', walletProvider)}
          </StyledLink>
        </StyledContent>
        <StyledDoneWrapper>
          <Button
            text={getLocale('done')}
            size={'medium'}
            type={'accent'}
            onClick={onClose}
          />
        </StyledDoneWrapper>
      </>
    )
  }

  getBackupNotice = () => {
    const {
      onClose,
      onVerify,
      walletProvider
    } = this.props

    return (
      <>
        <StyledContent>
          {getLocale('rewardsBackupNoticeText1')}
        </StyledContent>
        <StyledContent>
          {getLocale('rewardsBackupNoticeText2')}
          <StyledLink onClick={onVerify} id={'backup-verify-link'}>
            {getLocale('rewardsBackupNoticeText3').replace('$1', walletProvider)}
          </StyledLink>
        </StyledContent>
        <StyledDoneWrapper>
          <Button
            text={getLocale('done')}
            size={'medium'}
            type={'accent'}
            onClick={onClose}
          />
        </StyledDoneWrapper>
      </>
    )
  }

  getBackup = () => {
    if (this.props.showBackupNotice) {
      return this.getBackupNotice()
    } else {
      return this.getBackupLegacy()
    }
  }

  getRestore = () => {
    const { error, onClose, funds } = this.props
    const errorShown = error && this.state.errorShown

    return (
      <>
        {
          funds
          ? <StyledStatus>
              <Alert type={'warning'} colored={true} bg={true}>
                {getLocale('rewardsRestoreWarning', { funds: funds })}
              </Alert>
            </StyledStatus>
          : null
        }
        <ControlWrapper
          text={
            <>
              {getLocale('rewardsRestoreText4')} <StyledImport
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
            id={'backup-recovery-key'}
            fieldError={!!errorShown}
            value={this.state.recoveryKey}
            onChange={this.setRecoveryKey}
          />
        </ControlWrapper>
        {
          errorShown
          ? <StyledStatus isError={true}>
              <Alert type={'error'} colored={true} bg={true}>
                {error}
              </Alert>
            </StyledStatus>
          : null
        }
        <StyledTextWrapper>
          <StyledText>
            {getLocale('rewardsRestoreText3')}
          </StyledText>
        </StyledTextWrapper>
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
      </>
    )
  }

  confirmSelection = () => {
    const confirmed = confirm(getLocale('rewardsResetConfirmation'))
    if (confirmed === true) {
      this.props.onReset()
    }
  }

  getReset = () => {
    const parts = getLocale('rewardsResetTextFunds').split(/\$\d/g)
    return (
      <>
        <StyledTextWrapper>
          <StyledText data-test-id={'reset-text'}>
            {
              this.props.internalFunds > 0
              ? <span>
                {parts[0]}
                <b>{this.props.internalFunds.toString()} BAT</b>
                {parts[1]}
                {this.props.walletProvider}
                {parts[2]}
              </span>
              : getLocale('rewardsResetTextNoFunds')
            }
          </StyledText>
        </StyledTextWrapper>
        <StyledActionsWrapper>
          <ActionButton
            id={'reset-button'}
            level={'primary'}
            type={'accent'}
            text={getLocale('reset')}
            size={'medium'}
            onClick={this.confirmSelection}
          />
        </StyledActionsWrapper>
      </>
    )
  }

  getTabContent = (activeTabId: number) => {
    switch (activeTabId) {
      case 0: {
        return this.getBackup()
      }
      case 1: {
        return this.getRestore()
      }
      case 2: {
        return this.getReset()
      }
    }

    return null
  }

  render () {
    const {
      id,
      activeTabId,
      onClose,
      onTabChange,
      testId
    } = this.props

    return (
      <Modal id={id} onClose={onClose} size={'small'} testId={testId}>
        <StyledTitleWrapper>
          <StyledTitle>
            {getLocale('manageWallet')}
          </StyledTitle>
        </StyledTitleWrapper>
        <StyledControlWrapper>
          <Tab
            testId={'settings-modal-tabs'}
            onChange={onTabChange}
            tabIndexSelected={activeTabId}
            tabTitles={[
              getLocale('backup'),
              getLocale('restore'),
              getLocale('reset')
            ]}
          />
        </StyledControlWrapper>
        {
          this.getTabContent(activeTabId)
        }
      </Modal>
    )
  }
}
