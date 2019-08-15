// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { StyledWidgetMenuContainer, StyledWidgetMenu, StyledWidgetButton, StyledWidgetIcon, StyledSpan } from './styles'
import { IconButton } from '../../default'
import { CaratCircleODownIcon, CloseStrokeIcon } from 'brave-ui/components/icons'

interface Props {
  menuPosition: 'right' | 'left'
  hideWidget: () => void
  textDirection: string
  toggleWidgetHover: () => void
  widgetMenuPersist: boolean
  unpersistWidgetHover: () => void
}

interface State {
  showMenu: boolean
}

export default class WidgetMenu extends React.PureComponent<Props, State> {
  settingsMenuRef: React.RefObject<any>
  constructor (props: Props) {
    super(props)
    this.settingsMenuRef = React.createRef()
    this.state = {
      showMenu: false
    }
  }

  handleClickOutsideMenu = (event: Event) => {
    if (this.settingsMenuRef && !this.settingsMenuRef.current.contains(event.target)) {
      this.props.unpersistWidgetHover()
      this.closeMenu()
    }
  }

  componentDidMount () {
    document.addEventListener('mousedown', this.handleClickOutsideMenu)
  }

  componentWillUnmount () {
    document.removeEventListener('mousedown', this.handleClickOutsideMenu)
  }

  toggleMenu = () => {
    this.props.toggleWidgetHover()
    this.setState({ showMenu: !this.state.showMenu })
  }

  closeMenu = () => {
    this.setState({ showMenu: false })
  }

  unmountWidget = () => {
    this.props.hideWidget()
    this.props.unpersistWidgetHover()
    this.closeMenu()
  }

  render () {
    const { menuPosition, textDirection, widgetMenuPersist } = this.props
    const { showMenu } = this.state
    return (
      <StyledWidgetMenuContainer
        innerRef={this.settingsMenuRef}
        widgetMenuPersist={widgetMenuPersist}
      >
        <IconButton onClick={this.toggleMenu}><CaratCircleODownIcon/></IconButton>
        {showMenu && <StyledWidgetMenu
          textDirection={textDirection}
          menuPosition={menuPosition}
        >
          <StyledWidgetButton
            onClick={this.unmountWidget}
          >
            <StyledWidgetIcon><CloseStrokeIcon/></StyledWidgetIcon>
            <StyledSpan>Remove</StyledSpan>
          </StyledWidgetButton>
        </StyledWidgetMenu>}
      </StyledWidgetMenuContainer>
    )
  }
}
