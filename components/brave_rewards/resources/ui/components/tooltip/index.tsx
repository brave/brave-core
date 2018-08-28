/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/

import * as React from 'react'
import {
  StyledWrapper,
  StyledTooltip,
  StyledTooltipText,
  StyledPointer
} from './style'

export interface Props {
  id?: string
  content: React.ReactNode
  children?: React.ReactNode
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
    const { id, content, children } = this.props

    return (
      <StyledWrapper
        id={id}
        onMouseEnter={this.onMouseEnter}
        onMouseLeave={this.onMouseLeave}
      >
        <StyledTooltip displayed={this.state.displayed}>
          <StyledPointer/>
          <StyledTooltipText>
            {content}
          </StyledTooltipText>
        </StyledTooltip>
        {children}
      </StyledWrapper>
    )
  }
}
