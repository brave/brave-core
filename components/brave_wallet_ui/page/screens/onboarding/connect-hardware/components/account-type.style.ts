// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Icon from '@brave/leo/react/icon'
import * as leo from '@brave/leo/tokens/css'

export const Container = styled.button`
  display: flex;
  justify-content: flex-start;
  align-items: flex-start;
  padding: 24px;
  width: 100%;
  outline: none;
  border: none;
  background-color: transparent;
  cursor: pointer;
`

export const Title = styled.p`
  color: ${leo.color.text.primary};
  font-family: Poppins;
  font-size: 22px;
  font-style: normal;
  font-weight: 600;
  line-height: 28px;
  padding: 0;
  margin: 0;
`

export const Description = styled.p`
  width: 90%;
  color: ${leo.color.text.primary};
  font-family: Poppins;
  font-size: 14px;
  font-style: normal;
  font-weight: 400;
  line-height: 22px;
  padding: 0;
  margin: 0;
  text-align: left;
`

export const NetworkIcons = styled.div`
  display: flex;
  justify-content: flex-start;

  leo-icon {
    margin-left: -8px;
  }

  & > :first-child {
    margin-left: 0px;
  }
`

export const NetworkIcon = styled(Icon)`
  --leo-icon-size: 32px;
  border-radius: 50%;
  border: 2px solid ${leo.color.container.background};
`

export const ArrowIcon = styled(Icon).attrs({ name: 'arrow-right' })`
  --leo-icon-size: 24px;
  color: ${leo.color.icon.interactive};
`

export const Divider = styled.div`
  width: 100%;
  height: 1px;
  background: ${leo.color.divider.subtle};
`
