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
  StyledBalanceUnavailable,
  StyledGrantWrapper,
  StyledGrant,
  StyledActionWrapper,
  StyledBalanceCurrency,
  StyledCurve,
  StyledAlertWrapper,
  StyledAlertClose,
  StyleGrantButton,
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
  StyledLink
} from './style'
import { getLocale } from '../../../helpers'
import { GrantCaptcha, GrantComplete, GrantError, GrantWrapper, WalletPopup } from '../'
import Alert, { Type as AlertType } from '../alert'
import Button, { Props as ButtonProps } from '../../../components/buttonsIndicators/button'
import {
  CaratDownIcon,
  CaratUpIcon,
  CloseCircleOIcon,
  SettingsAdvancedIcon,
  UpholdColorIcon,
  UpholdSystemIcon,
  CaratCircleRightIcon
} from '../../../components/icons'

import giftIconUrl from './assets/gift.svg'
import loveIconUrl from './assets/love.svg'
import megaphoneIconUrl from './assets/megaphone.svg'

type Grant = {
  tokens: string,
  expireDate: string,
  type: string
}

type GrantClaim = {
  promotionId?: string
  altcurrency?: string
  probi: string
  expiryTime: number
  captcha?: string
  hint?: string
  status?: 'wrongPosition' | 'grantGone' | 'generalError' | 'grantAlreadyClaimed' | number | null
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
}

export type NotificationType =
  'ads' |
  'ads-launch' |
  'backupWallet' |
  'contribute' |
  'grant' |
  'insufficientFunds' |
  'tipsProcessed' |
  'error' |
  'pendingContribution' |
  'verifyWallet' |
  ''

export type WalletState =
  'unverified' |
  'verified' |
  'connected' |
  'disconnected_unverified' |
  'disconnected_verified'

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
  walletState?: WalletState
  compact?: boolean
  contentPadding?: boolean
  showCopy?: boolean
  children?: React.ReactNode
  showSecActions?: boolean
  onSettingsClick?: () => void
  onActivityClick?: () => void
  grants?: Grant[]
  grant?: GrantClaim
  alert?: AlertWallet | null
  id?: string
  gradientTop?: string
  isMobile?: boolean
  notification?: Notification
  onNotificationClick?: () => void
  onGrantHide?: () => void
  onFinish?: () => void
  onSolution?: (x: number, y: number) => void
  convertProbiToFixed?: (probi: string, place: number) => string
  onVerifyClick?: () => void
  onDisconnectClick?: () => void
  goToUphold?: () => void
  userName?: string
}

export type Step = '' | 'captcha' | 'complete'

interface State {
  grantDetails: boolean,
  verificationDetails: boolean
}

export default class WalletWrapper extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      grantDetails: false,
      verificationDetails: false
    }
  }

  generateActions (actions: { icon: React.ReactNode, name: string, testId?: string, action: () => void }[], id?: string) {
    return actions && actions.map((action, i: number) => {
      return (
        <StyledAction key={`${id}-${i}`} onClick={action.action} data-test-id={action.testId}>
          <StyledActionIcon>{action.icon}</StyledActionIcon>
          <StyledActionText>{action.name}</StyledActionText>
        </StyledAction>
      )
    })
  }

  onNotificationClick = () => {
    if (this.props.onNotificationClick) {
      this.props.onNotificationClick()
    }
  }

  onGrantHide = () => {
    if (this.props.onGrantHide) {
      this.props.onGrantHide()
    }
  }

  onFinish = () => {
    if (this.props.onFinish) {
      this.props.onFinish()
    }
  }

  onSolution = (x: number, y: number) => {
    if (this.props.onSolution) {
      this.props.onSolution(x, y)
    }
  }

  grantCaptcha = () => {
    const { grant } = this.props
    const status = grant && grant.status

    if (!grant || !grant.promotionId) {
      return
    }

    if (status === 'grantGone') {
      return (
        <GrantWrapper
          onClose={this.onFinish}
          title={getLocale('grantGoneTitle')}
          text={''}
        >
          <GrantError
            buttonText={getLocale('grantGoneButton')}
            text={getLocale('grantGoneText')}
            onButtonClick={this.onFinish}
          />
        </GrantWrapper>
      )
    }

    if (status === 'grantAlreadyClaimed') {
      return (
        <GrantWrapper
          onClose={this.onFinish}
          title={getLocale('grantGoneTitle')}
          text={''}
        >
          <GrantError
            buttonText={getLocale('grantGoneButton')}
            text={getLocale('grantAlreadyClaimedText')}
            onButtonClick={this.onFinish}
          />
        </GrantWrapper>
      )
    }

    if (status === 'generalError') {
      return (
        <GrantWrapper
          onClose={this.onGrantHide}
          title={getLocale('grantGeneralErrorTitle')}
          text={''}
        >
          <GrantError
            buttonText={getLocale('grantGeneralErrorButton')}
            text={getLocale('grantGeneralErrorText')}
            onButtonClick={this.onGrantHide}
          />
        </GrantWrapper>
      )
    }

    if (!grant.captcha || !grant.hint) {
      return
    }

    return (
      <GrantWrapper
        isPanel={true}
        onClose={this.onGrantHide}
        title={status === 'wrongPosition' ? getLocale('captchaMissedTarget') : getLocale('captchaProveHuman')}
        text={getLocale('proveHuman')}
        hint={grant.hint}
      >
        <GrantCaptcha
          isPanel={true}
          onSolution={this.onSolution}
          dropBgImage={grant.captcha}
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
      case 'grant':
        buttonText = getLocale('claim')
        buttonAction = this.onNotificationClick
        break
      case 'ads-launch':
        buttonText = getLocale('turnOnAds')
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
      default:
        buttonText = getLocale('ok').toUpperCase()
        break
    }

    return (
      <StyledButton
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
    const buttonProps: Partial<ButtonProps> = {
      size: 'small',
      level: 'primary',
      brand: 'rewards',
      onClick: this.props.onVerifyClick
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
            active={this.state.verificationDetails}
            id={'verified-wallet-button'}
          >
            <StyledVerifiedButtonIcon position={'before'}>
              <UpholdSystemIcon />
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
              image: <UpholdSystemIcon />,
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
    const { goToUphold, userName, onDisconnectClick, onVerifyClick, walletState } = this.props
    const verified = walletState === 'verified'
    const connected = walletState === 'connected'

    return (
      <WalletPopup
        onClose={this.toggleVerificationDetails}
        userName={userName || ''}
        verified={verified}
      >
        {
          <StyledDialogList>
            {
              connected
              ? <li>
                <StyledLink onClick={this.onDetailsLinkClicked.bind(this, onVerifyClick)} target={'_blank'}>
                  {getLocale('walletGoToVerifyPage')}
                </StyledLink>
              </li>
              : null
            }
            <li>
              <StyledLink onClick={this.onDetailsLinkClicked.bind(this, goToUphold)} target={'_blank'}>
                {getLocale('walletGoToUphold')}
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

  toggleGrantDetails = () => {
    this.setState({ grantDetails: !this.state.grantDetails })
  }

  hasGrants = (grants?: Grant[]) => {
    return grants && grants.length > 0
  }

  getNotificationIcon = (notification: Notification) => {
    let icon

    switch (notification.type) {
      case 'ads':
      case 'ads-launch':
      case 'backupWallet':
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
      case 'ads-launch':
        typeText = getLocale('braveAdsLaunchTitle')
        break
      case 'backupWallet':
        typeText = getLocale('backupWalletTitle')
        break
      case 'contribute':
        typeText = getLocale('braveContributeTitle')
        break
      case 'grant':
        typeText = getLocale('tokenGrant')
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

  render () {
    const {
      id,
      children,
      balance,
      converted,
      actions,
      showCopy,
      walletState,
      compact,
      contentPadding,
      showSecActions,
      grants,
      grant,
      onSettingsClick,
      alert,
      gradientTop,
      notification,
      isMobile,
      convertProbiToFixed,
      onVerifyClick,
      onDisconnectClick
    } = this.props

    const hasGrants = this.hasGrants(grants)

    let tokens = '0.0'
    if (grant && grant.probi && convertProbiToFixed) {
      tokens = convertProbiToFixed(grant.probi, 1)
    }

    let date = ''
    if (grant && grant.expiryTime !== 0) {
      date = new Date(grant.expiryTime).toLocaleDateString()
    }

    const walletVerified = walletState === 'verified' || walletState === 'disconnected_verified'
    const connectedVerified = walletState === 'verified'

    return (
      <>
        <StyledWrapper
          id={id}
          compact={compact}
          isMobile={isMobile}
          notification={notification}
        >
          {
            grant && !grant.probi
              ? this.grantCaptcha()
              : null
          }
          {
            grant && grant.probi
              ? <GrantWrapper
                isPanel={true}
                onClose={this.onFinish}
                title={grant.finishTitle || ''}
                text={grant.finishText}
              >
                <GrantComplete isMobile={true} onClose={this.onFinish} amount={tokens} date={date} tokenTitle={grant.finishTokenTitle} />
              </GrantWrapper>
              : null
          }
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
                    walletState
                      ? this.generateWalletButton(walletState)
                      : <StyledTitle>{getLocale('yourWallet')}</StyledTitle>
                  }
                  {
                    showSecActions
                      ? <StyledIconAction onClick={onSettingsClick} data-test-id='settingsButton'>
                        <SettingsAdvancedIcon />
                      </StyledIconAction>
                      : null
                  }
                  {
                    walletState && (walletState === 'disconnected_unverified' || walletState === 'disconnected_verified')
                      ? <StyledBalance>
                        <StyledBalanceUnavailable>{getLocale('balanceUnavailable')}</StyledBalanceUnavailable>
                        <StyleGrantButton>
                          <Button
                            text={getLocale('reconnectWallet')}
                            size={'small'}
                            type={'subtle'}
                            level={'secondary'}
                            onClick={onVerifyClick}
                          />
                        </StyleGrantButton>
                      </StyledBalance>
                      : <StyledBalance>
                        <StyledBalanceTokens data-test-id='balance'>
                          {balance} <StyledBalanceCurrency>BAT</StyledBalanceCurrency>
                        </StyledBalanceTokens>
                        {
                          converted
                            ? <StyledBalanceConverted>{converted}</StyledBalanceConverted>
                            : null
                        }
                        {
                          hasGrants
                            ? <StyleGrantButton>
                              <Button
                                text={getLocale('grants')}
                                size={'small'}
                                type={'subtle'}
                                level={'secondary'}
                                onClick={this.toggleGrantDetails}
                                icon={{ position: 'after', image: this.state.grantDetails ? <CaratUpIcon /> : <CaratDownIcon /> }}
                              />
                            </StyleGrantButton>
                            : null
                        }
                      </StyledBalance>
                  }
                  {
                    this.state.grantDetails && hasGrants
                      ? <StyledGrantWrapper>
                        {
                          grants && grants.map((grant: Grant, i: number) => {
                            return <StyledGrant key={`${id}-grant-${i}`}>
                              <b>{grant.tokens} BAT</b>
                              {
                                grant.type === 'ads'
                                ? <span>{getLocale('adsEarnings')}</span>
                                : <span>{getLocale('expiresOn')} {grant.expireDate}</span>
                              }
                            </StyledGrant>
                          })
                        }
                      </StyledGrantWrapper>
                      : null
                  }
                  <StyledActionWrapper>
                    {this.generateActions(actions, id)}
                  </StyledActionWrapper>
                </>
                : this.generateNotification(notification)
            }
            <StyledCurve background={gradientTop} />
          </StyledHeader>
          <StyledContent
            contentPadding={contentPadding}
          >
            {children}
          </StyledContent>
          {
            showCopy
              ? <StyledCopy connected={connectedVerified}>
                {
                  walletVerified
                    ? <>
                      <StyledCopyImage>
                        <UpholdColorIcon />
                      </StyledCopyImage>
                      {getLocale('rewardsPanelText1')} <b>Uphold</b>.
                    </>
                    : <>
                      <StyledCopyImage>
                        <UpholdSystemIcon />
                      </StyledCopyImage>
                      {getLocale('rewardsPanelText2')} <b>Uphold</b>.
                      {
                        onVerifyClick
                          ? <>
                            {' ('}
                            <StyledLink onClick={onVerifyClick}>
                              {getLocale('rewardsPanelTextVerify')}
                            </StyledLink>
                            {')'}
                          </>
                          : null
                      }
                    </>
                }
              </StyledCopy>
              : null
          }
        </StyledWrapper>
        {
          showCopy
            ? <StyledBAT>
              {getLocale('rewardsPanelText3')} <a href={'https://basicattentiontoken.org/'} target={'_blank'}>{getLocale('rewardsPanelText4')}</a>
            </StyledBAT>
            : null
        }
      </>
    )
  }
}
