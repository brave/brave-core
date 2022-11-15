// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import { NavButton } from '../../../../extension'

// Styled Components
import {
  StyledWrapper,
  InfoColumn,
  Title,
  Description,
  NetworkIcon,
  LeftSide
} from './style'

export interface Props {
  onClickCreate: () => void
  icon: string
  title: string
  description: string
  buttonText: string
}

const AccountTypeItem = (props: Props) => {
  const {
    title,
    description,
    buttonText,
    icon,
    onClickCreate
  } = props

  return (
    <StyledWrapper>
      <LeftSide>
        <NetworkIcon src={icon} />
        <InfoColumn>
          <Title>{title}</Title>
          <Description>{description}</Description>
        </InfoColumn>
      </LeftSide>
      <NavButton
        buttonType='secondary'
        onSubmit={onClickCreate}
        text={buttonText}
      />
    </StyledWrapper>
  )
}

export default AccountTypeItem
