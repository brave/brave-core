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
  StyledGrantWrapper,
  StyledGrant,
  StyledActionWrapper,
  StyledBalanceCurrency,
  StyledCurve,
  StyledAlertWrapper,
  StyledAlertClose,
  StyleGrantButton
} from './style'
import { getLocale } from '../../../helpers'
import Alert, { Type as AlertType } from '../alert'
import { Button } from '../../../components'

type Grant = {
  tokens: number,
  expireDate: string
}

export interface Props {
  tokens: number
  converted: string | null
  actions: {icon: string, name: string, action: () => void}[]
  connectedWallet?: boolean
  showCopy?: boolean
  children?: React.ReactNode
  showSecActions?: boolean
  onSettingsClick?: () => void
  onActivityClick?: () => void
  grants?: Grant[]
  alert?: {
    node: React.ReactNode
    type: AlertType
    onAlertClose?: () => void
  } | null
  id?: string
}

const upholdIcon = require('./assets/uphold')
const upholdColorIcon = require('./assets/upholdColor')
const gearIcon = require('./assets/gear')
const arrowUpIcon = require('./assets/arrowUp')
const arrowDownIcon = require('./assets/arrowDown')
const closeIcon = require('./assets/close')

interface State {
  grantDetails: boolean
}

export default class WalletWrapper extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      grantDetails: false
    }
  }

  formatTokens (tokens: number) {
    if (isNaN(tokens)) {
      return '00.00'
    }

    return tokens.toFixed(1)
  }

  generateActions (actions: {icon: string, name: string, action: () => void}[], id?: string) {
    return actions && actions.map((action, i: number) => {
      return (
        <StyledAction key={`${id}-${i}`} onClick={action.action}>
          <StyledActionIcon src={action.icon} />{action.name}
        </StyledAction>
      )
    })
  }

  toggleGrantDetails = () => {
    this.setState({ grantDetails: !this.state.grantDetails })
  }

  hasGrants = (grants?: Grant[]) => {
    return grants && grants.length > 0
  }

  render () {
    const {
      id,
      children,
      tokens,
      converted,
      actions,
      showCopy,
      connectedWallet,
      showSecActions,
      grants,
      onSettingsClick,
      alert
    } = this.props

    const enabled = this.hasGrants(grants)

    return (
      <StyledWrapper id={id}>
        <StyledHeader>
          {
            alert
            ? <StyledAlertWrapper>
              {
                alert.onAlertClose
                ? <StyledAlertClose onClick={alert.onAlertClose}>{closeIcon}</StyledAlertClose>
                : null
              }
              <Alert type={alert.type} bg={true}>
                  {alert.node}
              </Alert>
            </StyledAlertWrapper>
            : null
          }
          <StyledTitle>{getLocale('yourWallet')}</StyledTitle>
          {
            showSecActions
            ? <StyledIconAction onClick={onSettingsClick}>
              {gearIcon}
            </StyledIconAction>
            : null
          }

          <StyledBalance>
            <StyledBalanceTokens>
              {this.formatTokens(tokens)} <StyledBalanceCurrency>BAT</StyledBalanceCurrency>
            </StyledBalanceTokens>
            {
              converted
              ? <StyledBalanceConverted>{converted}</StyledBalanceConverted>
              : null
            }
            <StyleGrantButton>
              <Button
                text={getLocale('grants')}
                size={'small'}
                type={'subtle'}
                level={'secondary'}
                onClick={enabled ? this.toggleGrantDetails : undefined}
                disabled={!enabled}
                icon={{ position: 'after', image: this.state.grantDetails && enabled ? arrowUpIcon : arrowDownIcon }}
              />
            </StyleGrantButton>
          </StyledBalance>
          {
            this.state.grantDetails && enabled
            ? <StyledGrantWrapper>
              {
                grants && grants.map((grant: Grant, i: number) => {
                  return <StyledGrant key={`${id}-grant-${i}`}>
                    <b>{this.formatTokens(grant.tokens)} BAT</b> <span>{getLocale('expiresOn')} {grant.expireDate}</span>
                  </StyledGrant>
                })
              }
            </StyledGrantWrapper>
            : null
          }
          <StyledActionWrapper>
            {this.generateActions(actions, id)}
          </StyledActionWrapper>
          <StyledCurve />
        </StyledHeader>
        <StyledContent>
          {children}
        </StyledContent>
        {
          showCopy
          ? <StyledCopy connectedWallet={connectedWallet}>
              {
                connectedWallet
                ? <>
                    <StyledCopyImage>{upholdColorIcon}</StyledCopyImage> {getLocale('rewardsPanelText1')} <b>Uphold</b>.
                </>
                : <>
                    <StyledCopyImage>{upholdIcon}</StyledCopyImage> {getLocale('rewardsPanelText2')} <b>Uphold</b>.
                </>
              }
          </StyledCopy>
          : null
        }
      </StyledWrapper>
    )
  }
}
