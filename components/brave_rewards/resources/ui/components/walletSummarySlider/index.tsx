/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from '../../../helpers'

import {
  StyledWrapper,
  StyledTransitionWrapper,
  StyledSummaryText,
  StyledToggleWrapper,
  StyledArrowIcon,
  StyledGrid,
  StyledColumn
} from './style'

import {
  CaratCircleOUpIcon,
  CaratCircleODownIcon
} from '../../../components/icons'

export interface Props {
  id?: string
  onToggle?: () => void
  children?: React.ReactNode | React.ReactNode[]
}

interface State {
  panelOneShown: boolean
  panelTwoShown: boolean
}

export default class WalletSummarySlider extends React.PureComponent<
  Props,
  State
> {
  constructor (props: object) {
    super(props)
    this.state = {
      panelOneShown: true,
      panelTwoShown: false
    }
    this.togglePanels = this.togglePanels.bind(this)
  }

  togglePanels (e: any) {
    this.setState({
      panelOneShown: !this.state.panelOneShown,
      panelTwoShown: !this.state.panelTwoShown
    })

    if (this.props.onToggle) {
      this.props.onToggle()
    }
  }

  getPanel (panel: React.ReactNode, showTitle: boolean = false) {
    return (
      <StyledWrapper>
        {showTitle ? panel : null}
        <StyledToggleWrapper show={showTitle} onClick={this.togglePanels}>
          <StyledGrid>
            <StyledColumn size={'5'}>
              {showTitle ? (
                <StyledSummaryText>
                  {getLocale('rewardsSummary')}
                </StyledSummaryText>
              ) : null}
            </StyledColumn>
            <StyledColumn size={'1'}>
              <StyledArrowIcon show={showTitle}>
                {!showTitle ? <CaratCircleODownIcon /> : <CaratCircleOUpIcon />}
              </StyledArrowIcon>
            </StyledColumn>
          </StyledGrid>
        </StyledToggleWrapper>
        {!showTitle ? panel : null}
      </StyledWrapper>
    )
  }

  render () {
    const { id, children } = this.props

    if (!Array.isArray(children) || children[0] === null) {
      return <StyledWrapper id={id}>{children}</StyledWrapper>
    }

    if (!children || children.length !== 2) {
      return null
    }

    const panelOne = children[0]
    const panelTwo = children[1]

    return (
      <StyledWrapper id={id}>
        <StyledTransitionWrapper show={this.state.panelOneShown}>
          {this.getPanel(panelOne, true)}
        </StyledTransitionWrapper>
        <StyledTransitionWrapper show={this.state.panelTwoShown}>
          {this.getPanel(panelTwo)}
        </StyledTransitionWrapper>
      </StyledWrapper>
    )
  }
}
