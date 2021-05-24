/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

import * as rewardsPanelActions from '../actions/rewards_panel_actions'

import { getLocale } from 'brave-ui/helpers'

import braveSpinner from './assets/brave-spinner.svg'
import checkIcon from './assets/check.svg'
import smileySadIcon from './assets/smiley-sad.svg'

import {
  StyledBorderlessButton,
  StyledButton,
  StyledCaptchaFrame,
  StyledIcon,
  StyledTitle,
  StyledText,
  StyledValidationSpinner,
  StyledValidationText,
  StyledWrapper
} from './adaptive_captcha_panel.style'

interface Props extends RewardsExtension.ComponentProps {
}

interface State {
  showMessage: 'success' | 'maxAttemptsExceeded' | 'validating' | 'none'
}

export class AdaptiveCaptchaPanel extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      showMessage: this.props.rewardsPanelData.scheduledCaptcha.maxAttemptsExceeded ?
                   'maxAttemptsExceeded' : 'none'
    }
  }

  componentDidMount () {
    this.getScheduledCaptchaInfo()

    window.addEventListener('message', (event) => {
      // Sandboxed iframes which lack the 'allow-same-origin' header have "null"
      // rather than a valid origin
      if (event.origin !== 'null') {
        return
      }

      const captchaFrame = document.getElementById('scheduled-captcha') as HTMLIFrameElement
      if (!captchaFrame) {
        return
      }

      const captchaContentWindow = captchaFrame.contentWindow
      if (!event.source || event.source !== captchaContentWindow) {
        return
      }

      if (!event.data) {
        return
      }

      switch (event.data) {
        case 'captchaSuccess':
          this.setState({ showMessage: 'success' })
          chrome.braveRewards.updateScheduledCaptchaResult(true)
          break
        case 'captchaFailure':
        case 'error':
          chrome.braveRewards.updateScheduledCaptchaResult(false)
          break
      }

      this.getScheduledCaptchaInfo()
    })
  }

  componentDidUpdate (prevProps: Props, prevState: State) {
    if (!prevProps.rewardsPanelData.scheduledCaptcha.maxAttemptsExceeded &&
        this.props.rewardsPanelData.scheduledCaptcha.maxAttemptsExceeded) {
      this.setState({ showMessage: 'maxAttemptsExceeded' })
    }
  }

  getScheduledCaptchaInfo = () => {
    chrome.braveRewards.getScheduledCaptchaInfo((result: boolean, scheduledCaptcha: RewardsExtension.ScheduledCaptcha) => {
      this.props.actions.onGetScheduledCaptchaInfo(result, scheduledCaptcha)
    })
  }

  onClose = () => {
    this.setState({ showMessage: 'none' })
    //    this.props.onCaptchaDismissed()
    window.close()
  }

  onContactSupport = () => {
    this.setState({ showMessage: 'none' })
    window.open('https://support.brave.com/', '_blank')
  }

  getScheduledCaptcha = () => {
    return (
      <StyledCaptchaFrame
        id='scheduled-captcha'
        src={this.props.rewardsPanelData.scheduledCaptcha.url}
        sandbox='allow-scripts'
        scrolling='no'
      />
    )
  }

  getValidatingInterstitial = () => {
    return (
      <StyledWrapper>
        <StyledValidationSpinner src={braveSpinner} />
        <StyledValidationText>
          {getLocale('validating')}
        </StyledValidationText>
      </StyledWrapper>
    )
  }

  getMaxAttemptsExceededMessage = () => {
    return (
      <StyledWrapper>
        <StyledIcon src={smileySadIcon} />
        <StyledTitle>
          {getLocale('captchaMaxAttemptsExceededTitle')}
        </StyledTitle>
        <StyledText>
          {getLocale('captchaMaxAttemptsExceededText')}
        </StyledText>
        <StyledButton onClick={this.onContactSupport}>
          {getLocale('contactSupport')}
        </StyledButton>
      </StyledWrapper>
    )
  }

  getSuccessMessage = () => {
    return (
      <StyledWrapper>
        <StyledIcon src={checkIcon} />
        <StyledTitle textSize='large'>
          {getLocale('captchaSolvedTitle')}
        </StyledTitle>
        <StyledText>
          {getLocale('captchaSolvedText')}
        </StyledText>
        <StyledBorderlessButton onClick={this.onClose}>
          {getLocale('dismiss')}
        </StyledBorderlessButton>
      </StyledWrapper>
    )
  }

  render () {
    switch (this.state.showMessage) {
      case 'success':
        return this.getSuccessMessage()
      case 'maxAttemptsExceeded':
        return this.getMaxAttemptsExceededMessage()
      case 'validating':
        return this.getValidatingInterstitial()
      case 'none':
        break
    }

    return this.getScheduledCaptcha()
  }
}

export const mapStateToProps = (state: RewardsExtension.ApplicationState) => ({
  rewardsPanelData: state.rewardsPanelData
})

export const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(rewardsPanelActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(AdaptiveCaptchaPanel)
