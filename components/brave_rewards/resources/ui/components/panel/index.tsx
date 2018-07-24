/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledWrapper,
  StyledHeader,
  StyledTitle,
  StyledBalance,
  StyledBalanceTitle,
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
  StyledActionWrapper
} from './style'
import ButtonSecondary from '../../../components/buttonsIndicators/buttonSecondary/index'
import { getLocale } from '../../../helpers'

type Grant = {tokens: number, expireDate: string}

export interface Props {
  tokens: number
  converted: string
  actions: {icon: string, name: string, action: () => void}[]
  connectedWallet?: boolean
  showCopy?: boolean
  children?: React.ReactNode
  showSecActions?: boolean
  onSettingsClick?: () => void
  onActivityClick?: () => void
  grants?: Grant[]
  id?: string
}

const panel = require('./assets/panel')
const upholdIcon = require('./assets/uphold')
const upholdColorIcon = require('./assets/upholdColor')
const gearIcon = require('./assets/gear')
const arrowUpIcon = require('./assets/arrowUp')
const arrowDownIcon = require('./assets/arrowDown')

interface State {
  grantDetails: boolean
}

export default class Panel extends React.PureComponent<Props, State> {
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

    return tokens.toFixed(2)
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
      onSettingsClick
    } = this.props

    const enabled = this.hasGrants(grants)

    return (
      <StyledWrapper id={id}>
        <StyledHeader bg={panel}>
          <StyledTitle>{getLocale('yourWallet')}</StyledTitle>
          {
            showSecActions
            ? <StyledIconAction onClick={onSettingsClick}>
              {gearIcon}
            </StyledIconAction>
            : null
          }

          <StyledBalance>
            <StyledBalanceTitle>{getLocale('tokenBalance')}</StyledBalanceTitle>
            <StyledBalanceTokens>{this.formatTokens(tokens)}</StyledBalanceTokens>
            <StyledBalanceConverted>~ {converted}</StyledBalanceConverted>
            <ButtonSecondary
              text={getLocale('detail')}
              size={'small'}
              color={'subtle'}
              onClick={enabled ? this.toggleGrantDetails : undefined}
              disabled={!enabled}
              icon={{ position: 'right', image: this.state.grantDetails && enabled ? arrowUpIcon : arrowDownIcon }}
            />
          </StyledBalance>
          {
            this.state.grantDetails && enabled
            ? <StyledGrantWrapper>
              {
                grants && grants.map((grant: Grant, i: number) => {
                  return <StyledGrant key={`${id}-grant-${i}`}>
                    <b>{grant.tokens} {getLocale('tokens')}</b> <span>{getLocale('expiresOn')} {grant.expireDate}</span>
                  </StyledGrant>
                })
              }
            </StyledGrantWrapper>
            : null
          }
          <StyledActionWrapper>
            {this.generateActions(actions, id)}
          </StyledActionWrapper>
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
