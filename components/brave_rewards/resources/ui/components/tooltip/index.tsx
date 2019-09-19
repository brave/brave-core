/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/

import * as React from 'react'
import {
  StyledWrapper,
  StyledTooltip,
  StyledTooltipText,
  StyledPointer,
  StyledChildWrapper
} from './style'

export interface Props {
  id?: string
  content: React.ReactNode
  children?: React.ReactNode
  rightEdge?: boolean
}

interface State {
  displayed: boolean
}

export default class Tooltip extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)

    this.state = {
      displayed: false
    }
  }

  onMouseEnter = () => {
    this.setState({ displayed: true })
  }

  onMouseLeave = () => {
    this.setState({ displayed: false })
  }

  render () {
    const { id, content, children, rightEdge } = this.props

    return (
      <StyledWrapper id={id}>
        <StyledTooltip
          displayed={this.state.displayed}
          rightEdge={rightEdge || false}
        >
          <StyledPointer rightEdge={rightEdge || false}/>
          <StyledTooltipText>
            {content}
          </StyledTooltipText>
        </StyledTooltip>
        <StyledChildWrapper
          onMouseEnter={this.onMouseEnter}
          onMouseLeave={this.onMouseLeave}
        >
          {children}
        </StyledChildWrapper>
      </StyledWrapper>
    )
  }
}
