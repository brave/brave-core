/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import {
  StyledTitle,
  StyledDesc,
  StyledInfoCard,
  StyledFigure,
  StyledGrid,
  StyledColumn
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
  getCards (items: CardProps[]) {
    return (
      <StyledGrid>
        {items.map((item: CardProps, index: number) => {
          return <StyledColumn key={`${index}`}>
            <StyledInfoCard>
              <StyledFigure>
                {item.icon}
              </StyledFigure>
              <StyledTitle>{item.title}</StyledTitle>
              <StyledDesc>{item.description}</StyledDesc>
            </StyledInfoCard>
          </StyledColumn>
        })}
      </StyledGrid>
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
