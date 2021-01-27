/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledWrapper,
  StyledHeader,
  StyledTitle,
  StyledBalance,
  StyledBalanceTokens,
  StyledContent,
  StyledAction,
  StyledActionIcon,
  StyledCopy,
  StyledCopyImage,
  StyledIconAction,
  StyledBalanceConverted,
  StyledActionWrapper,
  StyledBalanceCurrency,
  StyledCurve,
  StyledAlertWrapper,
  StyledAlertClose,
  StyledActionText,
  StyledBAT,
  StyledNotificationIcon,
  StyledNotificationCloseIcon,
  StyledTypeText,
  StyledMessageText,
  StyledDateText,
  StyledNotificationContent,
  StyledButtonWrapper,
  StyledButton,
  StyledNotificationMessage,
  StyledPipe,
  StyledVerifiedButton,
  StyledVerifiedButtonIcon,
  StyledVerifiedButtonText,
  StyledDialogList,
  StyledLink,
  LoginMessage,
  LoginMessageButtons,
  LoginMessageText
} from './style'
import { getLocale } from 'brave-ui/helpers'
import { GrantCaptcha, GrantComplete, GrantError, GrantWrapper, WalletPopup } from '../'
import Alert, { Type as AlertType } from '../alert'
import Button, { Props as ButtonProps } from 'brave-ui/components/buttonsIndicators/button'
import {
  CaratDownIcon,
  CloseCircleOIcon,
  SettingsAdvancedIcon,
  UpholdColorIcon,
  UpholdSystemIcon,
  CaratCircleRightIcon
} from 'brave-ui/components/icons'

import { BitflyerIcon } from '../../../shared/components/icons/bitflyer_icon'

import giftIconUrl from './assets/gift.svg'
import loveIconUrl from './assets/love.svg'
import megaphoneIconUrl from './assets/megaphone.svg'

type GrantStatus = 'start' | 'wrongPosition' | 'generalError' | 'finished' | null

type GrantClaim = {
  promotionId?: string
  amount: number
  expiresAt: number,
  type: number
  status: number
  captchaImage?: string
  hint?: string
  captchaStatus?: GrantStatus
  finishTokenTitle?: string
  finishTitle?: string
  finishText?: string
}

export interface AlertWallet {
  node: React.ReactNode
  type: AlertType
  onAlertClose?: () => void
}

export interface ActionWallet {
  icon: React.ReactNode
  name: string
  action: () => void
  testId?: string
  externalWallet: boolean
}

export type NotificationType =
  'ads' |
  'backupWallet' |
  'contribute' |
  'deviceLimitReached' |
  'error' |
  'grant' |
  'insufficientFunds' |
  'pendingContribution' |
  'tipsProcessed' |
  'verifyWallet' |
  ''

export type WalletState =
  'unverified' |
  'verified' |
  'connected' |
  'disconnected_unverified' |
  'disconnected_verified' |
  'pending'

export interface Notification {
  id: string
  date?: string
  type: NotificationType
  text: React.ReactNode
  onCloseNotification: (id: string) => void
}

export interface Props {
  balance: string
  converted: string | null
  actions: ActionWallet[]
  walletType?: string
  walletState?: WalletState
  walletProvider: string
  compact?: boolean
  contentPadding?: boolean
  showCopy?: boolean
  children?: React.ReactNode
  showSecActions?: boolean
  onSettingsClick?: () => void
  grant?: GrantClaim
  alert?: AlertWallet | null
  id?: string
  gradientTop?: string
  isMobile?: boolean
  notification?: Notification
  onNotificationClick?: () => void
  onGrantHide?: (promotionId: string) => void
  onFinish?: (promotionId: string) => void
  onSolution?: (promotionId: string, x: number, y: number) => void
  onVerifyClick?: () => void
  onDisconnectClick?: () => void
  goToExternalWallet?: () => void
  greetings?: string
  onlyAnonWallet?: boolean
  showLoginMessage?: boolean
}

export type Step = '' | 'captcha' | 'complete'

interface State {
  verificationDetails: boolean
  showLoginMessage: boolean
}

export default class WalletWrapper extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      verificationDetails: false,
      showLoginMessage: false
    }
  }

  generateActions (actions: ActionWallet[], id?: string) {
    return actions && actions.map((action, i: number) => {
      let clickAction = action.externalWallet ? this.onActionClick.bind(this, action.action) : action.action
      return (
        <StyledAction key={`${id}-${i}`} onClick={clickAction} data-test-id={action.testId}>
          <StyledActionIcon>{action.icon}</StyledActionIcon>
          <StyledActionText>{action.name}</StyledActionText>
        </StyledAction>
      )
    })
  }

  onActionClick = (action: () => void) => {
    if (!action) {
      return
    }

    if (!this.props.showLoginMessage) {
      action()
      return
    }

    this.setState({
      showLoginMessage: true
    })
  }

  onNotificationClick = () => {
    if (this.props.onNotificationClick) {
      this.props.onNotificationClick()
    }
  }

  onGrantHide = (promotionId: string) => {
    if (this.props.onGrantHide) {
      this.props.onGrantHide(promotionId)
    }
  }

  onFinish = (promotionId: string) => {
    if (this.props.onFinish) {
      this.props.onFinish(promotionId)
    }
  }

  onSolution = (promotionId: string, x: number, y: number) => {
    if (this.props.onSolution) {
      this.props.onSolution(promotionId, x, y)
    }
  }

  grantCaptcha = () => {
    const { grant } = this.props
    const status = grant && grant.captchaStatus

    if (!grant || !grant.promotionId) {
      return
    }

    if (status === 'generalError') {
      return (
        <GrantWrapper
          onClose={this.onGrantHide.bind(this, grant.promotionId)}
          title={getLocale('grantGeneralErrorTitle')}
          text={''}
        >
          <GrantError
            buttonText={getLocale('grantGeneralErrorButton')}
            text={getLocale('grantGeneralErrorText')}
            onButtonClick={this.onGrantHide.bind(this, grant.promotionId)}
          />
        </GrantWrapper>
      )
    }

    if (!grant.captchaImage || !grant.hint) {
      return
    }

    return (
      <GrantWrapper
        isPanel={true}
        onClose={this.onGrantHide.bind(this, grant.promotionId)}
        title={status === 'wrongPosition' ? getLocale('captchaMissedTarget') : getLocale('captchaProveHuman')}
        text={getLocale('proveHuman')}
        hint={grant.hint}
      >
        <GrantCaptcha
          isPanel={true}
          onSolution={this.onSolution.bind(this, grant.promotionId)}
          captchaImage={grant.captchaImage}
          hint={grant.hint}
          isWindows={window.navigator.platform === 'Win32'}
        />
      </GrantWrapper>
    )
  }

  getNotificationButton = (type: NotificationType, onClose: any) => {
    let buttonText = 'OK'
    let buttonAction = onClose

    switch (type) {
      case 'ads':
      case 'grant':
        buttonText = getLocale('claim')
        buttonAction = this.onNotificationClick
        break
      case 'backupWallet':
        buttonText = getLocale('backupNow')
        buttonAction = this.onNotificationClick
        break
      case 'insufficientFunds':
        buttonText = getLocale('addFunds')
        buttonAction = this.onNotificationClick
        break
      case 'verifyWallet':
        buttonText = getLocale('whyHow').toUpperCase()
        buttonAction = this.onNotificationClick
        break
      case 'deviceLimitReached':
        buttonText = getLocale('deviceLimitReachedLearnMore')
        buttonAction = this.onNotificationClick
        break
      default:
        buttonText = getLocale('ok').toUpperCase()
        break
    }

    return (
      <StyledButton
        id={'panel-notifications'}
        size={'small'}
        type={'accent'}
        level={'primary'}
        onClick={buttonAction}
        text={buttonText}
      />
    )
  }

  generateNotification = (notification: Notification | undefined) => {
    if (!notification) {
      return null
    }

    const onClose = notification.onCloseNotification.bind(this, notification.id)

    return (
      <>
        <StyledNotificationCloseIcon data-test-id={'notification-close'} onClick={onClose}>
          <CloseCircleOIcon />
        </StyledNotificationCloseIcon>
        <StyledNotificationContent>
          {this.getNotificationIcon(notification)}
          {this.getNotificationMessage(notification)}

          <StyledButtonWrapper>
            {this.getNotificationButton(notification.type, onClose)}
          </StyledButtonWrapper>
        </StyledNotificationContent>
      </>
    )
  }

  generateWalletButton = (walletState: WalletState) => {
    const { walletType } = this.props
    const walletProviderIcon =
      walletType === 'uphold' ? <UpholdSystemIcon /> :
      walletType === 'bitflyer' ? <BitflyerIcon white={true} /> :
      null

    const buttonProps: Partial<ButtonProps> = {
      size: 'small',
      level: 'primary',
      brand: 'rewards',
      onClick: this.onActionClick.bind(this, this.props.onVerifyClick)
    }

    switch (walletState) {
      case 'unverified':
        return (
          <Button
            type={'accent'}
            icon={{
              image: <CaratCircleRightIcon />,
              position: 'after'
            }}
            text={getLocale('walletButtonUnverified')}
            {...buttonProps}
            id={'verify-wallet-button'}
          />
        )

      case 'verified':
        return (
          <StyledVerifiedButton
            onClick={this.toggleVerificationDetails}
            id={'verified-wallet-button'}
          >
            <StyledVerifiedButtonIcon position={'before'}>
              {walletProviderIcon}
            </StyledVerifiedButtonIcon>
            <StyledVerifiedButtonText>
              {getLocale('walletButtonVerified')}
            </StyledVerifiedButtonText>
            <StyledVerifiedButtonIcon position={'after'}>
              <CaratDownIcon />
            </StyledVerifiedButtonIcon>
          </StyledVerifiedButton>
        )

      case 'connected':
      case 'pending':
        return (
          <Button
            type={'accent'}
            icon={{
              image: <CaratDownIcon />,
              position: 'after'
            }}
            text={getLocale('walletButtonUnverified')}
            {...buttonProps}
            onClick={this.toggleVerificationDetails}
            id={'verify-wallet-button'}
          />
        )

      case 'disconnected_unverified':
      case 'disconnected_verified':
        return (
          <Button
            text={getLocale('walletButtonDisconnected')}
            type={'subtle'}
            icon={{
              image: walletProviderIcon,
              position: 'before'
            }}
            {...buttonProps}
            id={'disconnected-wallet-button'}
          />
        )
    }
  }

  toggleVerificationDetails = () => {
    this.setState({ verificationDetails: !this.state.verificationDetails })
  }

  onDetailsLinkClicked = (action?: () => void) => {
    if (action) {
      action()
    }
    this.toggleVerificationDetails()
  }

  getVerificationDetails = () => {
    const { goToExternalWallet, greetings, onDisconnectClick, onVerifyClick, walletState, walletType, walletProvider } = this.props
    const notVerified = walletState === 'connected' || walletState === 'pending'

    return (
      <WalletPopup
        onClose={this.toggleVerificationDetails}
        greetings={greetings || ''}
        walletState={walletState}
        walletType={walletType}
      >
        {
          <StyledDialogList>
            {
              notVerified
              ? <li>
                <StyledLink onClick={this.onDetailsLinkClicked.bind(this, onVerifyClick)} target={'_blank'}>
                  {getLocale('walletGoToVerifyPage')}
                </StyledLink>
              </li>
              : null
            }
            <li>
              <StyledLink onClick={this.onDetailsLinkClicked.bind(this, goToExternalWallet)} target={'_blank'}>
                {getLocale('walletGoToProvider').replace('$1', walletProvider)}
              </StyledLink>
            </li>
            <li>
              <StyledLink onClick={this.onDetailsLinkClicked.bind(this, onDisconnectClick)}>
                {getLocale('walletDisconnect')}
              </StyledLink>
            </li>
          </StyledDialogList>
        }
      </WalletPopup>
    )
  }

  getNotificationIcon = (notification: Notification) => {
    let icon

    switch (notification.type) {
      case 'ads':
      case 'backupWallet':
      case 'deviceLimitReached':
      case 'insufficientFunds':
      case 'verifyWallet':
        icon = megaphoneIconUrl
        break
      case 'contribute':
      case 'tipsProcessed':
      case 'pendingContribution':
        icon = loveIconUrl
        break
      case 'grant':
        icon = giftIconUrl
        break
      default:
        icon = ''
        break
    }

    if (!icon) {
      return null
    }

    return (
      <StyledNotificationIcon src={icon} />
    )
  }

  getNotificationMessage = (notification: Notification) => {
    let typeText

    switch (notification.type) {
      case 'ads':
        typeText = getLocale('braveAdsTitle')
        break
      case 'backupWallet':
        typeText = getLocale('backupWalletTitle')
        break
      case 'contribute':
        typeText = getLocale('braveContributeTitle')
        break
      case 'deviceLimitReached':
        typeText = getLocale('deviceLimitReachedTitle')
        break
      case 'grant':
        typeText = this.props.onlyAnonWallet
          ? getLocale('pointGrants')
          : getLocale('tokenGrants')
        break
      case 'insufficientFunds':
        typeText = getLocale('insufficientFunds')
        break
      case 'tipsProcessed':
        typeText = getLocale('contributionTips')
        break
      case 'pendingContribution':
        typeText = getLocale('pendingContributionTitle')
        break
      case 'verifyWallet':
        typeText = getLocale('verifyWalletTitle')
        break
      default:
        typeText = ''
        break
    }

    return (
      <StyledNotificationMessage>
        <StyledTypeText>{typeText}</StyledTypeText> <StyledPipe>|</StyledPipe>
        <StyledMessageText>{notification.text}</StyledMessageText>
        {
          notification.date
          ? <StyledDateText>{notification.date}</StyledDateText>
          : null
        }
      </StyledNotificationMessage>
    )
  }

  toggleLoginMessage = () => {
    this.setState({
      showLoginMessage: false
    })
  }

  render () {
    const {
      id,
      children,
      balance,
      converted,
      actions,
      showCopy,
      walletType,
      walletState,
      walletProvider,
      compact,
      contentPadding,
      showSecActions,
      grant,
      onSettingsClick,
      alert,
      gradientTop,
      notification,
      isMobile,
      onVerifyClick,
      onDisconnectClick,
      onlyAnonWallet
    } = this.props

    let date = ''
    let grantAmount = '0.000'

    if (grant) {
      if (grant.expiresAt) {
        date = new Date(grant.expiresAt).toLocaleDateString()
      }

      if (grant.amount) {
        grantAmount = grant.amount.toFixed(3)
      }

      if (grant.captchaImage && grant.captchaStatus !== 'finished') {
        let rendered = this.grantCaptcha()
        if (rendered) {
          return rendered
        }
      }

      if (grant.captchaStatus === 'finished') {
        return (
          <GrantWrapper
            isPanel={true}
            onClose={this.onFinish.bind(this, grant.promotionId)}
            title={grant.finishTitle || ''}
            text={grant.finishText}
          >
            <GrantComplete
              isMobile={true}
              onClose={this.onFinish.bind(this, grant.promotionId)}
              amount={grantAmount}
              date={date}
              tokenTitle={grant.finishTokenTitle}
              onlyAnonWallet={onlyAnonWallet}
            />
          </GrantWrapper>
        )
      }
    }

    const walletVerified = walletState === 'verified' || walletState === 'disconnected_verified'
    const connectedVerified = walletState === 'verified'
    const batFormatString = onlyAnonWallet ? getLocale('batPoints') : getLocale('bat')

    const upholdLoginText = getLocale('loginMessageText').split('$1')
    const rewardsText1 = getLocale('rewardsPanelText1').split(/\$\d/g)
    const rewardsText2 = getLocale('rewardsPanelText2').split(/\$\d/g)

    const walletIcon =
      walletType === 'uphold' ? <UpholdColorIcon /> :
      null

    return (
      <>
        <StyledWrapper
          id={id}
          compact={compact}
          isMobile={isMobile}
          notification={notification}
        >
          <StyledHeader>
            {
              !notification
                ? <>
                  {
                    this.state.verificationDetails && onDisconnectClick
                      ? this.getVerificationDetails()
                      : null
                  }
                  {
                    alert && alert.node
                      ? <StyledAlertWrapper>
                        {
                          alert.onAlertClose
                            ? <StyledAlertClose onClick={alert.onAlertClose}>
                              <CloseCircleOIcon />
                            </StyledAlertClose>
                            : null
                        }
                        <Alert type={alert.type} bg={true}>
                          {alert.node}
                        </Alert>
                      </StyledAlertWrapper>
                      : null
                  }
                  {
                    walletState && !onlyAnonWallet
                      ? this.generateWalletButton(walletState)
                      : <StyledTitle>{onlyAnonWallet ? getLocale('yourBalance') : getLocale('yourWallet')}</StyledTitle>
                  }
                  {
                    showSecActions
                      ? <StyledIconAction onClick={onSettingsClick} data-test-id='settingsButton'>
                        <SettingsAdvancedIcon />
                      </StyledIconAction>
                      : null
                  }
                  <StyledBalance>
                    <StyledBalanceTokens data-test-id='balance'>
                      {balance} <StyledBalanceCurrency>{batFormatString}</StyledBalanceCurrency>
                    </StyledBalanceTokens>
                    {
                      converted
                        ? <StyledBalanceConverted>{converted}</StyledBalanceConverted>
                        : null
                    }
                  </StyledBalance>
                  <StyledActionWrapper>
                    {this.generateActions(actions, id)}
                  </StyledActionWrapper>
                </>
                : this.generateNotification(notification)
            }
            <StyledCurve background={gradientTop} />
            {
              this.state.showLoginMessage
              ? <LoginMessage>
                  <LoginMessageText>
                    <b>{getLocale('loginMessageTitle')}</b>
                    <p>
                      {upholdLoginText[0]}
                      <br />
                      {upholdLoginText[1]}
                    </p>
                    <br/>
                    {getLocale('walletVerificationNote3').replace('$1', walletProvider)}
                  </LoginMessageText>
                  <LoginMessageButtons>
                    <Button
                      level={'secondary'}
                      type={'accent'}
                      text={getLocale('cancel')}
                      onClick={this.toggleLoginMessage}
                      id={'cancel-login-button'}
                    />
                    <Button
                      level={'primary'}
                      type={'accent'}
                      text={getLocale('login')}
                      onClick={this.props.onVerifyClick}
                      id={'login-button'}
                    />
                  </LoginMessageButtons>
              </LoginMessage>
              : null
            }
          </StyledHeader>
          <StyledContent
            contentPadding={contentPadding}
          >
            {children}
          </StyledContent>
          {
            showCopy && !onlyAnonWallet
              ? <StyledCopy connected={connectedVerified}>
                {
                  walletVerified
                  ?
                    <>
                      <StyledCopyImage>{walletIcon}</StyledCopyImage>
                      <span>
                        {rewardsText1[0]}
                        <b>{walletProvider}</b>
                        {rewardsText1[1]}
                      </span>
                    </>
                  : walletType === 'uphold' && <>
                      <StyledCopyImage>{walletIcon}</StyledCopyImage>
                      <span>
                        {rewardsText2[0]}
                        <b>{walletProvider}</b>
                        {rewardsText2[1]}
                      </span>
                      {
                        onVerifyClick && <>
                          {' ('}
                          <StyledLink onClick={this.onActionClick.bind(this, this.props.onVerifyClick)}>
                            {getLocale('rewardsPanelTextVerify')}
                          </StyledLink>
                          {')'}
                        </>
                      }
                    </>
                }
              </StyledCopy>
              : null
          }
        </StyledWrapper>
        {
          showCopy && !onlyAnonWallet
            ? <StyledBAT>
              {getLocale('rewardsPanelText3')} <a href={'https://basicattentiontoken.org/'} target={'_blank'}>{getLocale('rewardsPanelText4')}</a>
            </StyledBAT>
            : null
        }
      </>
    )
  }
}
