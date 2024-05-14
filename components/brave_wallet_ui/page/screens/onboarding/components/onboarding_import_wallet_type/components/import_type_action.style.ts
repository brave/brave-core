// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Icon from '@brave/leo/react/icon'
import * as leo from '@brave/leo/tokens/css/variables'

export const ActionWrapper = styled.button`
  display: flex;
  flex-direction: column;
  justify-content: start;
  width: 100%;
  padding: 24px;
  border-radius: 8px;
  background-color: transparent;
  cursor: pointer;
  border: none;
  outline: 1px solid transparent;
`

export const RightArrow = styled(Icon).attrs({
  name: 'arrow-right'
})`
  --leo-icon-size: 24px;
  margin-left: 8px;
  color: ${leo.color.button.background};
  align-self: self-end;
`

export const Title = styled.h3`
  color: ${leo.color.text.primary};
  font: ${leo.font.heading.h3};
  width: 100%;
  text-align: left;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  margin: 0;
  padding: 0;
`

export const Subtitle = styled.h5`
  color: ${leo.color.text.secondary};
  font: ${leo.font.default.regular};
  width: 100%;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  text-align: left;
  margin: 0;
  padding: 0;
`

export const ActionIcon = styled(Icon)`
  display: flex;
  align-items: center;
  justify-content: center;
  --leo-icon-size: 24px;
`

export const IconsWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-wrap: wrap;
  gap: 12px;
`
