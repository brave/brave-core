/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as CSS from 'csstype'
import { StyledWrapper, StyledTooltip, StyledArrow, StyledArrowOutline } from './style'

export interface Theme {
  color?: {
    background?: CSS.ColorProperty
    arrow?: CSS.Color
    text?: CSS.Color
    border?: CSS.Color
  }
  border?: CSS.BorderProperty<1>
  padding?: CSS.PaddingProperty<1>
  offSet?: number
  width?: CSS.WidthProperty<1>
  boxShadow?: CSS.BoxShadowProperty
  align?: CSS.TextAlignProperty
}

export interface Props {
  content: React.ReactNode
  id?: string
  position?: 'top' | 'bottom' | 'left' | 'right'
  customStyle?: Theme
  children?: React.ReactNode
}

interface State {
  open: boolean
}

/*
  FIND SOME PLUGIN
  maybe https://github.com/tvkhoa/react-tippy
 */
export default class Tooltip extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)

    this.state = {
      open: false
    }
  }
  onMouseEnter = () => {
    this.setState({ open: true })
  }

  onMouseLeave = () => {
    this.setState({ open: false })
  }

  get position () {
    let position = this.props.position

    if (!position) {
      position = 'bottom'
    }

    return position
  }

  render () {
    const { id, content, children, customStyle } = this.props

    let position = this.position

    return (
      <StyledWrapper id={id} onMouseEnter={this.onMouseEnter} onMouseLeave={this.onMouseLeave}>
        <StyledTooltip position={position} customStyle={customStyle} open={this.state.open}>
          <StyledArrow position={position} customStyle={customStyle} />
          <StyledArrowOutline position={position} customStyle={customStyle} />
          {content}
        </StyledTooltip>
        {children}
      </StyledWrapper>
    )
  }
}
