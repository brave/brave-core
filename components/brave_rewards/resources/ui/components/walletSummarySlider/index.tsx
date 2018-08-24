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
  StyledArrowIcon
} from './style'

import { Grid, Column } from '../../../components'
import { CaratUpIcon, CaratDownIcon } from '../../../components/icons'

export interface Props {
  id?: string
  children?: React.ReactNode[]
}

interface State {
  panelOneShown: boolean,
  panelTwoShown: boolean
}

export default class WalletSummarySlider extends React.PureComponent<Props, State> {
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
  }

  getPanel (panel: React.ReactNode, showTitle: boolean = false) {
    return (
      <StyledWrapper>
        {
          showTitle
          ? panel
          : null
        }
        <StyledToggleWrapper
          show={showTitle}
          onClick={this.togglePanels}
        >
          <Grid columns={6}>
            <Column size={5}>
              {
                showTitle
                ? <StyledSummaryText>
                    {getLocale('rewardsSummary')}
                  </StyledSummaryText>
                : null
              }
            </Column>
            <Column size={1}>
              <StyledArrowIcon show={showTitle}>
                {
                  showTitle
                  ? <CaratUpIcon color={'#696FDC'}/>
                  : <CaratDownIcon color={'#696FDC'}/>
                }
              </StyledArrowIcon>
            </Column>
          </Grid>
        </StyledToggleWrapper>
        {
          !showTitle
          ? panel
          : null
        }
      </StyledWrapper>
    )
  }

  render () {
    const { id, children } = this.props

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
