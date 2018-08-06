/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as CSS from 'csstype'
import { StyledWrapper, StyledTitle, StyledContentWrapper } from './style'
import Tokens from '../tokens/index'

export interface Props {
  title: React.ReactNode
  value: number
  converted: number
  id?: string
  theme?: Theme
  isNegative?: boolean
}

interface Theme {
  color?: CSS.Color
  fontWeight?: CSS.FontWeightProperty
  borderTop?: CSS.BorderTopProperty<1>
  borderBottom?: CSS.BorderTopProperty<1>
  marginBottom?: CSS.MarginBottomProperty<1>
}

export default class ListToken extends React.PureComponent<Props, {}> {
  render () {
    const { id, title, value, theme, converted, isNegative } = this.props

    return (
      <StyledWrapper id={id} theme={theme}>
        <StyledTitle theme={theme}>{title}</StyledTitle>
        <StyledContentWrapper>
          <Tokens
            value={value}
            converted={converted}
            isNegative={isNegative}
            theme={{
              color: {
                tokenNum: theme && theme.color || '#686978',
                token: '#686978',
                text: '#9E9FAB'
              },
              size: {
                text: '10px',
                token: '12px',
                tokenNum: '14px'
              }
            }}
          />
        </StyledContentWrapper>
      </StyledWrapper>
    )
  }
}
