/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
import * as React from 'react'

// Utils
import { BraveWallet, BuyOption } from '../../constants/types'

// Styled Components
import {
  Logo,
  Content,
  StyledWrapper,
  Name,
  Description,
  StyledButton,
  ButtonText
} from './buy-option-item-styles'

interface Props {
  option: BuyOption
  onSelect: (option: BraveWallet.OnRampProvider) => void
}
export const BuyOptionItem = (props: Props) => {
  const { option, onSelect } = props
  const { id, icon, name, description, actionText } = option

  const onClick = React.useCallback(() => {
    onSelect(id)
  }, [onSelect, id])

  return (
    <StyledWrapper>
      <Logo src={icon} />
      <Content>
        <Name>{name}</Name>
        <Description>{description}</Description>
        <StyledButton onClick={onClick}>
          <ButtonText>{actionText}</ButtonText>
        </StyledButton>
      </Content>
    </StyledWrapper>
  )
}
