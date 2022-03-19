/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { formatMessage } from '../../../shared/lib/locale_context'
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
  StyledTextWrapper,
  StyledSyncCodeWidget,
  StyledSyncCodeText,
  StyledSyncCodeFooter,
  StyledSyncCodeClipboard,
  StyledClipboardSuccess,
  StyledClipBoardButton
} from './style'
import { TextArea, Modal, Button } from 'brave-ui/components'
import { getLocale } from 'brave-ui/helpers'
import { Alert, Tab } from '../'
import ControlWrapper from 'brave-ui/components/formControls/controlWrapper'

export interface Props {
  backupKey: string
  activeTabId: number
  showBackupNotice: boolean
  onTabChange: (newTabId: number) => void
  onClose: () => void
  onCopy?: (key: string) => void
  onPrint?: (key: string) => void
  onSaveFile?: (key: string) => void
  onRestore: (key: string) => void
  onVerify?: () => void
  onShowQRCode: () => void
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

export function ClipBoardIcon () {
  return (
    <svg
      viewBox='0 0 32 32'
      className='icon'
    >
      <path d="M16 2c-1.645 0-3 1.355-3 3H9C7.355 5 6 6.355 6 8v18c0 1.645 1.355 3 3 3h14c1.645 0 3-1.355 3-3V8c0-1.645-1.355-3-3-3h-4c0-1.645-1.355-3-3-3zm0 2c.564 0 1 .436 1 1 0 .564-.436 1-1 1-.564 0-1-.436-1-1 0-.564.436-1 1-1zM9 7h4v1a1 1 0 0 0 1 1h4a1 1 0 0 0 1-1V7h4c.565 0 1 .435 1 1v18c0 .565-.435 1-1 1H9c-.565 0-1-.435-1-1V8c0-.565.435-1 1-1zm3 6a1 1 0 1 0 0 2h8a1 1 0 1 0 0-2h-8zm0 4a1 1 0 1 0 0 2h5a1 1 0 1 0 0-2h-5zm0 4a1 1 0 1 0 0 2h8a1 1 0 1 0 0-2h-8z" />
    </svg>
  )
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
      onVerify
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
          {
            formatMessage(getLocale('rewardsBackupNoticeText2'), {
              tags: {
                $1: (content) => (
                  <StyledLink key='link' onClick={onVerify}>
                    {content}
                  </StyledLink>
                )
              }
            })
          }
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
      onVerify
    } = this.props

    return (
      <>
        <StyledContent>
          {getLocale('rewardsBackupNoticeText1')}
        </StyledContent>
        <StyledContent>
          {
            formatMessage(getLocale('rewardsBackupNoticeText2'), {
              tags: {
                $1: (content) => (
                  <StyledLink key='link' onClick={onVerify} id={'backup-verify-link'}>
                    {content}
                  </StyledLink>
                )
              }
            })
          }
        </StyledContent>
        <StyledSyncCodeWidget>
          <StyledSyncCodeText readOnly={true} defaultValue='sync code goes here'></StyledSyncCodeText>
          <StyledSyncCodeFooter>
            {'24'}
            <StyledSyncCodeClipboard>
              <StyledClipboardSuccess>
                {'sync code copied'}
              </StyledClipboardSuccess>
              <StyledClipBoardButton>
                <ClipBoardIcon />
              </StyledClipBoardButton>
            </StyledSyncCodeClipboard>
          </StyledSyncCodeFooter>
        </StyledSyncCodeWidget>
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
    const { error, onShowQRCode, onClose, funds } = this.props
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
        <StyledTextWrapper>
          <StyledText>
            <StyledLink onClick={onShowQRCode}>
              {getLocale('rewardsViewQRCodeText1')}
            </StyledLink> {getLocale('rewardsViewQRCodeText2')}
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
    if (confirmed) {
      this.props.onReset()
    }
  }

  getReset = () => {
    const getText = () => {
      if (this.props.internalFunds <= 0) {
        return getLocale('rewardsResetTextNoFunds')
      }

      return formatMessage(getLocale('rewardsResetTextFunds'), {
        placeholders: {
          $1: (
            <b key='amount'>
              {this.props.internalFunds.toString()} BAT
            </b>
          )
        },
        tags: {
          $2: (content) => (
            <StyledLink key='link' onClick={this.props.onVerify}>
              {content}
            </StyledLink>
          )
        }
      })
    }

    return (
      <>
        <StyledTextWrapper>
          <StyledText data-test-id={'reset-text'}>
            {getText()}
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
