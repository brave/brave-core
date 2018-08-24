/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Grid, Column } from '../../../components/layout/gridList'
import {
  StyledTitle,
  StyledDesc,
  StyledInfoCard,
  StyledFigure
} from './style'

export interface CardProps {
  title?: string
  description?: string
  icon?: string
}

export interface Props {
  id?: string
  cards?: CardProps[]
}

export default class InfoCard extends React.PureComponent<Props, {}> {
  get gridTheme () {
    return {
      gridGap: '0'
    }
  }

  getCards (items: CardProps[]) {
    return (
      <Grid customStyle={this.gridTheme} columns={items.length}>
        {items.map((item: CardProps, index: number) => {
          return <Column key={`${index}`} size={1}>
            <StyledInfoCard>
              <StyledFigure>
                {item.icon}
              </StyledFigure>
              <StyledTitle>{item.title}</StyledTitle>
              <StyledDesc>{item.description}</StyledDesc>
            </StyledInfoCard>
          </Column>
        })}
      </Grid>
    )
  }

  render () {
    const { id, cards } = this.props
    return (
      <section id={id}>
        {
          cards
          ? this.getCards(cards)
          : null
        }
      </section>
    )
  }
}
