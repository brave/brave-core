// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/nala/react/icon'

// Styled Components
import { Column, Text, Row } from '$wallet/components/shared/style'
import {
  WelcomeActionWrapper,
  IconWrapper,
  WalletIcon,
} from './welcome_action.style'

interface Props {
  iconName: string
  title: string
  description: string
  walletIcons?: Array<string | React.ReactNode>
  onSelect: () => void
}

export const WelcomeAction = ({
  iconName,
  title,
  description,
  walletIcons,
  onSelect,
}: Props) => {
  return (
    <WelcomeActionWrapper onClick={onSelect}>
      <Row
        justifyContent='space-between'
        alignItems='center'
      >
        <IconWrapper>
          <Icon name={iconName} />
        </IconWrapper>
        <Row
          justifyContent='flex-start'
          gap='6px'
          padding='0px 10px 0px 0px'
          width='unset'
        >
          {walletIcons?.map((icon, index) => (
            <React.Fragment key={index}>
              {typeof icon === 'string' ? (
                <WalletIcon name={icon} />
              ) : (
                <React.Fragment>{icon}</React.Fragment>
              )}
            </React.Fragment>
          ))}
        </Row>
      </Row>
      <Column
        justifyContent='flex-start'
        alignItems='flex-start'
      >
        <Text
          variant='heading.h4'
          textColor='primary'
          textAlign='left'
        >
          {title}
        </Text>
        <Text
          variant='small.regular'
          textColor='tertiary'
          textAlign='left'
        >
          {description}
        </Text>
      </Column>
    </WelcomeActionWrapper>
  )
}
