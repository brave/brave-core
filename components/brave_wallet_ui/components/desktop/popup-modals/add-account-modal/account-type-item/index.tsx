// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import Icon from '@brave/leo/react/icon'

// Styled Components
import { StyledWrapper, Title, Description, NetworkIcon } from './style'
import { Row, Column } from '../../../../shared/style'

export interface Props {
  onClickCreate: () => void
  icon: string
  title: string
  description: string
}

const AccountTypeItem = (props: Props) => {
  const { title, description, icon, onClickCreate } = props

  return (
    <StyledWrapper
      justifyContent='space-between'
      gap='16px'
      role='button'
      onClick={onClickCreate}
    >
      <Row
        justifyContent='flex-start'
        alignItems='center'
        width='unset'
        gap='16px'
      >
        <NetworkIcon src={icon} />
        <Column alignItems='flex-start'>
          <Title textColor='primary'>{title}</Title>
          <Description
            textColor='secondary'
            textAlign='left'
          >
            {description}
          </Description>
        </Column>
      </Row>
      <Icon name='carat-right' />
    </StyledWrapper>
  )
}

export default AccountTypeItem
