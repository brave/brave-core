/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { formatMessage } from '../../../shared/lib/locale_context'
import {
  StyledContent,
  StyledActionsWrapper,
  StyledDoneWrapper,
  StyledLink,
  ActionButton,
  StyledTitle,
  StyledTitleWrapper,
  StyledControlWrapper,
  StyledText,
  StyledTextWrapper
} from './style'
import { Button, Checkbox, Modal } from 'brave-ui/components'
import { getLocale } from 'brave-ui/helpers'
import { Tab } from '../'

export interface Props {
  activeTabId: number
  onTabChange: (newTabId: number) => void
  onClose: () => void
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
  resetConsent: boolean
  errorShown: boolean
}

/*
  TODO
  - add error flow
 */
export default class ModalBackupReset extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      recoveryKey: '',
      resetConsent: false,
      errorShown: false
    }
  }

  setRecoveryKey = (event: React.ChangeEvent<HTMLTextAreaElement>) => {
    this.setState({
      errorShown: false,
      recoveryKey: event.target.value
    })
  }

  componentWillReceiveProps (nextProps: Props) {
    if (nextProps.error) {
      this.setState({
        errorShown: true
      })
    }
  }

  getBackup = () => {
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

  confirmSelection = () => {
    const confirmed = confirm(getLocale('rewardsResetConfirmation'))
    if (confirmed) {
      this.props.onReset()
    }
  }

  getReset = () => {
    const visitSupportURL = (event: React.UIEvent) => {
      window.open('https://support.brave.com/hc/en-us/articles/10007969237901', '_blank', 'noopener')
      event.stopPropagation()
    }

    const onResetConsentChange = () => {
      this.setState({
        resetConsent: !this.state.resetConsent
      })
    }

    const getResetText = () => {
      return formatMessage(getLocale('rewardsResetText'), {
        tags: {
          $1: (content) => (
            <StyledLink key='link' onClick={visitSupportURL}>
              {content}
            </StyledLink>
          )
        }
      })
    }

    const getConsentText = () => {
      return formatMessage(getLocale('rewardsResetConsent'), {
        tags: {
          $1: (content) => (
            <StyledLink key='link' onClick={visitSupportURL}>
              {content}
            </StyledLink>
          )
        }
      })
    }

    const getFundsWarningText = () => {
      return formatMessage(getLocale('rewardsResetTextFunds'), {
        placeholders: {
          $1: (
            <b key='amount'>
              {this.props.internalFunds.toString()} BAT
            </b>
          )
        }
      })
    }

    return (
      <>
        <StyledTextWrapper>
          <StyledText data-test-id={'reset-text'}>
            {getResetText()}
          </StyledText>
        </StyledTextWrapper>
        {
          this.props.internalFunds > 0 &&
          <StyledTextWrapper>
            <StyledText data-test-id={'funds-warning-text'}>
              {getFundsWarningText()}
            </StyledText>
          </StyledTextWrapper>
        }
        <Checkbox
          value={{ resetConsent: this.state.resetConsent }}
          onChange={onResetConsentChange}
        >
          <StyledTextWrapper data-key="resetConsent">
            <StyledText data-test-id={'funds-warning-text'}>
              {getConsentText()}
            </StyledText>
          </StyledTextWrapper>
        </Checkbox>
        <StyledActionsWrapper>
          <ActionButton
            id={'reset-button'}
            disabled={!this.state.resetConsent}
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
