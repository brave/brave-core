/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import createWidget from '../widget/index'
import { StyledTitleTab } from '../widgetTitleTab'

import {
  WidgetWrapper,
  IntroTitle,
  Copy,
  ActionsWrapper,
  ConnectButton,
  DismissAction,
  Header,
  StyledTitle,
  GeminiIcon,
  StyledTitleText
} from './style'
import GeminiLogo from './assets/gemini-logo'

// Utils
import { getLocale } from '../../../../common/locale'

interface State {

}

interface Props {
  showContent: boolean
  userAuthed: boolean
  authInProgress: boolean
  geminiClientUrl: string
  onShowContent: () => void
  onDisableWidget: () => void
  onValidAuthCode: () => void
  onConnectGemini: () => void
  onUpdateActions: () => void
  onGeminiClientUrl: (url: string) => void
}

class Gemini extends React.PureComponent<Props, State> {
  private refreshInterval: any

  constructor (props: Props) {
    super(props)
  }

  componentDidMount () {
    const { userAuthed, authInProgress } = this.props

    if (userAuthed) {
      this.props.onUpdateActions()
    }

    if (authInProgress) {
      this.checkForOauthCode()
    }

    this.getClientURL()
  }

  componentDidUpdate (prevProps: Props) {
    if (!prevProps.userAuthed && this.props.userAuthed) {
      this.props.onUpdateActions()
      this.checkSetRefreshInterval()
    }

    if (prevProps.userAuthed && !this.props.userAuthed) {
      this.getClientURL()
      this.clearIntervals()
    }
  }

  componentWillUnmount () {
    clearInterval(this.refreshInterval)
  }

  checkSetRefreshInterval = () => {
    if (!this.refreshInterval) {
      this.refreshInterval = setInterval(() => {
        this.props.onUpdateActions()
      }, 30000)
    }
  }

  clearIntervals = () => {
    clearInterval(this.refreshInterval)
  }

  getClientURL = () => {
    chrome.gemini.getClientUrl((clientUrl: string) => {
      this.props.onGeminiClientUrl(clientUrl)
    })
  }

  checkForOauthCode = () => {
    const params = window.location.search
    const urlParams = new URLSearchParams(params)
    const geminiAuth = urlParams.get('geminiAuth')

    if (geminiAuth) {
      chrome.gemini.getAccessToken((success: boolean) => {
        if (success) {
          this.props.onValidAuthCode()
        }
      })
    }
  }

  connectGemini = () => {
    window.open(this.props.geminiClientUrl, '_self', 'noopener')
    this.props.onConnectGemini()
  }

  renderIndexView () {
    return false
  }

  renderAuthView () {
    const { onDisableWidget } = this.props

    return (
      <>
        <IntroTitle>
          {'Purchase and trade with Gemini'}
        </IntroTitle>
        <Copy>
          {'Enable a Gemini connection to view your Gemini account balance and trade crypto.'}
        </Copy>
        <ActionsWrapper>
          {
            <>
              <ConnectButton onClick={this.connectGemini}>
                {getLocale('Connect to Gemini')}
              </ConnectButton>
              <DismissAction onClick={onDisableWidget}>
                {'No thank you'}
              </DismissAction>
            </>
          }
        </ActionsWrapper>
      </>
    )
  }

  renderRoutes () {
    const { userAuthed } = this.props

    if (userAuthed) {
      return (<p>You're logged in champ</p>)
    }

    return this.renderAuthView()
  }

  renderTitle () {
    return (
      <Header>
        <StyledTitle>
          <GeminiIcon>
            <GeminiLogo />
          </GeminiIcon>
          <StyledTitleText>
            {'Gemini'}
          </StyledTitleText>
        </StyledTitle>
      </Header>
    )
  }

  renderTitleTab () {
    const { onShowContent } = this.props

    return (
      <StyledTitleTab onClick={onShowContent}>
        {this.renderTitle()}
      </StyledTitleTab>
    )
  }

  render () {
    const { showContent, userAuthed } = this.props

    if (!showContent) {
      return this.renderTitleTab()
    }

    return (
      <WidgetWrapper tabIndex={0} userAuthed={userAuthed}>
        {
          this.renderIndexView()
          ? this.renderIndexView()
          : <>
              {this.renderTitle()}
              {this.renderRoutes()}
            </>
        }
      </WidgetWrapper>
    )
  }
}

export const GeminiWidget = createWidget(Gemini)
