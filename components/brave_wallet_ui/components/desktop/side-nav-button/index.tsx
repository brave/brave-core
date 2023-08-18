// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Styled Components
import { StyledButton, ButtonText, ButtonIcon } from './style'

export interface Props {
  isSelected: boolean
  text: string
  onSubmit: () => void
  icon: string
}

export class SideNavButton extends React.PureComponent<Props, {}> {
  render () {
    const { onSubmit, text, isSelected, icon } = this.props
    return (
      <StyledButton isSelected={isSelected} onClick={onSubmit}>
        <ButtonIcon icon={icon} />
        <ButtonText isSelected={isSelected}>{text}</ButtonText>
      </StyledButton>
    )
  }
}

export default SideNavButton