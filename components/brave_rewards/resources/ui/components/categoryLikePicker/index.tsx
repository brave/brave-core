/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledCategoryActionOptinLogo,
  StyledCategoryActionOptoutLogo,
  StyledCategoryActionOptinButton,
  StyledCategoryActionOptoutButton,
  StyledCategoryActionOptinFilledButton,
  StyledCategoryActionOptoutFilledButton
} from './style'
import {
  HeartLIcon,
  HeartSIcon,
  BlockLIcon,
  BlockSIcon
} from 'brave-ui/components/icons'
import { Tooltip } from '..'
import { getLocale } from 'brave-ui/helpers'

interface State {
  itemSelected: number
}

export interface Props {
  id?: string
  optAction: number
  onOptIn?: () => void
  onOptOut?: () => void
}

export default class ThumbLikePicker extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      itemSelected: props.optAction
    }
  }

  componentWillReceiveProps (nextProps: Props) {
    if ('optAction' in nextProps) {
      this.setState({ itemSelected: nextProps.optAction })
    }
  }

  showCategoryLike = () => {
    return (
      <>
        <StyledCategoryActionOptinLogo>
          <StyledCategoryActionOptinFilledButton onClick={this.props.onOptIn}>
            <HeartSIcon />
          </StyledCategoryActionOptinFilledButton>
        </StyledCategoryActionOptinLogo>
        <StyledCategoryActionOptoutLogo>
          <Tooltip
            content={getLocale('optOutTooltip')}
            rightEdge={true}
          >
            <StyledCategoryActionOptoutButton onClick={this.props.onOptOut}>
              <BlockLIcon />
            </StyledCategoryActionOptoutButton>
          </Tooltip>
        </StyledCategoryActionOptoutLogo>
      </>
    )
  }

  showCategoryBlock = () => {
    return (
      <>
        <StyledCategoryActionOptinLogo>
          <StyledCategoryActionOptinButton onClick={this.props.onOptIn}>
            <HeartLIcon />
          </StyledCategoryActionOptinButton>
        </StyledCategoryActionOptinLogo>
        <StyledCategoryActionOptoutLogo>
          <Tooltip
            content={getLocale('optOutTooltip')}
            rightEdge={true}
          >
            <StyledCategoryActionOptoutFilledButton onClick={this.props.onOptOut}>
              <BlockSIcon />
            </StyledCategoryActionOptoutFilledButton>
          </Tooltip>
        </StyledCategoryActionOptoutLogo>
      </>
    )
  }

  showCategoryUnselected = () => {
    return (
      <>
        <StyledCategoryActionOptinLogo>
          <StyledCategoryActionOptinButton onClick={this.props.onOptIn}>
            <HeartLIcon />
          </StyledCategoryActionOptinButton>
        </StyledCategoryActionOptinLogo>
        <StyledCategoryActionOptoutLogo>
          <Tooltip
            content={getLocale('optOutTooltip')}
            rightEdge={true}
          >
            <StyledCategoryActionOptoutButton onClick={this.props.onOptOut}>
              <BlockLIcon />
            </StyledCategoryActionOptoutButton>
          </Tooltip>
        </StyledCategoryActionOptoutLogo>
      </>
    )
  }

  render () {
    return (
      this.state.itemSelected === 1 ?
        this.showCategoryLike()
      :
        this.state.itemSelected === 2 ?
          this.showCategoryBlock()
      :
        this.showCategoryUnselected()
    )
  }
}
