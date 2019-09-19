/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledAdStatThumbUpIcon,
  StyledAdStatThumbUpFilledIcon,
  StyledAdStatThumbDownIcon,
  StyledAdStatThumbDownFilledIcon
} from './style'

import {
  ThumbsupLIcon,
  ThumbsdownLIcon,
  ThumbsupSIcon,
  ThumbsdownSIcon
} from 'brave-ui/components/icons'

interface State {
  itemSelected: number
}

export interface Props {
  id?: string
  likeAction: number
  onThumbUpPress?: () => void
  onThumbDownPress?: () => void
}

export default class ThumbLikePicker extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      itemSelected: props.likeAction
    }
  }

  componentWillReceiveProps (nextProps: Props) {
    if ('likeAction' in nextProps) {
      this.setState({ itemSelected: nextProps.likeAction })
    }
  }

  render () {
    return (
      <>
      {
        this.state.itemSelected === 1 ?
          <>
            <StyledAdStatThumbUpFilledIcon onClick={this.props.onThumbUpPress}>
              <ThumbsupSIcon />
            </StyledAdStatThumbUpFilledIcon>
            <StyledAdStatThumbDownIcon onClick={this.props.onThumbDownPress}>
              <ThumbsdownLIcon />
            </StyledAdStatThumbDownIcon>
          </>
        :
        this.state.itemSelected === 2 ?
          <>
            <StyledAdStatThumbUpIcon onClick={this.props.onThumbUpPress}>
              <ThumbsupLIcon />
            </StyledAdStatThumbUpIcon>
            <StyledAdStatThumbDownFilledIcon onClick={this.props.onThumbDownPress}>
              <ThumbsdownSIcon />
            </StyledAdStatThumbDownFilledIcon>
          </>
        :
          <>
            <StyledAdStatThumbUpIcon onClick={this.props.onThumbUpPress}>
              <ThumbsupLIcon />
            </StyledAdStatThumbUpIcon>
            <StyledAdStatThumbDownIcon onClick={this.props.onThumbDownPress}>
              <ThumbsdownLIcon />
            </StyledAdStatThumbDownIcon>
          </>
      }
      </>
    )
  }
}
