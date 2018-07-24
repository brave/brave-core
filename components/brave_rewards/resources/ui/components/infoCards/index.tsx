/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Grid, Column } from '../../../components/layout/gridList'
import {
  StyledTitle,
  StyledDesc,
  StyledInfoCard,
  StyledImage,
  StyledFigure
} from './style'

export interface InfoCardProps {
  title?: string
  description?: string
  icon?: string
}

export interface InfoCardsProps {
  id?: string
  infoItems?: InfoCardProps[]
}

class InfoCard extends React.PureComponent<InfoCardProps, {}> {

  render () {
    const { title, description, icon } = this.props
    return (
      <StyledInfoCard>
        <StyledFigure>
          <StyledImage src={icon}/>
        </StyledFigure>
        <StyledTitle>{title}</StyledTitle>
        <StyledDesc>{description}</StyledDesc>
      </StyledInfoCard>
    )
  }
}

export default class InfoCards extends React.PureComponent<InfoCardsProps, {}> {

  get gridTheme () {
    return {
      gridGap: '0px',
      maxWidth: '900px',
      margin: '0 auto'
    }
  }

  getCards (items: InfoCardProps[]) {
    return (
      <Grid theme={this.gridTheme} columns={items.length}>
        {items.map((item: InfoCardProps, index: number) => {
          return <Column key={`${index}`} size={1}>
              <InfoCard
                title={item.title}
                description={item.description}
                icon={item.icon}
              />
            </Column>
        })}
      </Grid>
    )
  }

  render () {
    const { id, infoItems } = this.props
    return (
      <section id={id}>
        {infoItems
          ? <div>
              {this.getCards(infoItems)}
            </div>
          : null}
      </section>
    )
  }
}

export {
  InfoCard,
  InfoCards
}
