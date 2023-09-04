/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { formatMessage } from '../../../shared/lib/locale_context'
import {
  StyledActionsWrapper,
  StyledLink,
  ActionButton,
  StyledTitle,
  StyledTitleWrapper,
  StyledText,
  StyledTextWrapper
} from './style'
import { Checkbox, Modal } from 'brave-ui/components'
import { getLocale } from 'brave-ui/helpers'

export interface Props {
  onClose: () => void
  error?: React.ReactNode
  id?: string
  testId?: string
  funds?: string
  onReset: () => void
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
export default class ModalReset extends React.PureComponent<Props, State> {
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

    return (
      <>
        <StyledTextWrapper>
          <StyledText data-test-id={'reset-text'}>
            {getResetText()}
          </StyledText>
        </StyledTextWrapper>
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
            onClick={this.props.onReset}
          />
        </StyledActionsWrapper>
      </>
    )
  }

  render () {
    const {
      id,
      onClose,
      testId
    } = this.props

    return (
      <Modal id={id} onClose={onClose} size={'small'} testId={testId}>
        <StyledTitleWrapper>
          <StyledTitle>
            {getLocale('resetWallet')}
          </StyledTitle>
        </StyledTitleWrapper>
        {
          this.getReset()
        }
      </Modal>
    )
  }
}
