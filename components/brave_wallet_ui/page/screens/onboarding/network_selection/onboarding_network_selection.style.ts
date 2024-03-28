// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

// styles
import { ScrollableColumn as ScrollableCol } from '../../../../components/shared/style'

export const GroupingText = styled.h3`
  color: ${leo.color.text.tertiary};
  font-family: 'Inter', 'Poppins';
  font-size: 12px;
  font-style: normal;
  font-weight: 600;
  line-height: 20px;
`

export const SelectAllText = styled.a`
  color: ${leo.color.text.interactive};
  font-family: 'Poppins';
  font-size: 12px;
  font-style: normal;
  font-weight: 600;
  line-height: 20px;
  letter-spacing: 0.36px;
  cursor: pointer;
`

export const ScrollableColumn = styled(ScrollableCol)`
  overflow-x: hidden;

  &::-webkit-scrollbar {
    width: 6px;
  }

  &::-webkit-scrollbar-thumb {
    background-color: ${leo.color.gray[20]};
    border-radius: 100px;
  }

  &::-webkit-scrollbar-track {
    background-color: ${leo.color.page.background};
  }
`

export const NetworkSelectionGrid = styled.div`
  display: grid;
  grid-template-columns: repeat(4, 105px);
  gap: ${leo.spacing.m};
  wrap: wrap;
`

export const NetworkSelectionContainer = styled.button`
  position: relative;
  display: flex;
  flex-direction: column;
  width: 105px;
  height: 105px;
  align-items: center;
  gap: 4px;
  border-radius: 8px;
  border: 1px solid ${leo.color.button.background};
  padding: ${leo.spacing.xl};
  background-color: transparent;
  cursor: pointer;
  overflow: hidden;

  leo-checkbox {
    position: absolute;
    top: 4px;
    right: 4px;
  }
`

