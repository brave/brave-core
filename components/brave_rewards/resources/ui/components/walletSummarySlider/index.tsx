/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from '../../../helpers'

import {
  StyledWrapper,
  StyledSummaryText,
  StyledArrowIcon,
  StyledNoTitleWrapper,
  StyledNoTitleArrowIcon
} from './style'
import { Grid, Column } from '../../../components'

export interface Props {
  id?: string
  title?: boolean
  onToggle: () => void
}

const arrowDownIcon = require('./assets/arrowDown')

export default class WalletSummarySlider extends React.PureComponent<Props, {}> {
  render () {
    const { id, onToggle, title } = this.props

    if (!title) {
      return (
        <StyledNoTitleWrapper
          id={id}
          onClick={onToggle}
        >
          <Grid columns={6}>
            <Column size={5}/>
            <Column size={1}>
              <StyledNoTitleArrowIcon>
                {arrowDownIcon}
              </StyledNoTitleArrowIcon>
            </Column>
          </Grid>
        </StyledNoTitleWrapper>
      )
    }

    return (
      <StyledWrapper
        id={id}
        onClick={onToggle}
      >
        <Grid columns={6}>
          <Column size={5}>
            {
              title
              ? <StyledSummaryText>
                  {getLocale('rewardsSummary')}
                </StyledSummaryText>
              : null
            }
          </Column>
          <Column size={1}>
            <StyledArrowIcon>
              {arrowDownIcon}
            </StyledArrowIcon>
          </Column>
        </Grid>
      </StyledWrapper>
    )
  }
}
