// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as nala from '@brave/nala/tokens/css/variables'

// Shared styles
import { ScrollableColumn as ScrollableCol } from '$wallet/components/shared/style'

export const SectionHeader = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  margin: ${nala.spacing['2Xl']} 0 ${nala.spacing.m};
  gap: ${nala.spacing.l};
`

export const GroupingText = styled.h3`
  margin: 0;
  font: ${nala.font.small.semibold};
  color: ${nala.color.text.tertiary};
`

export const Section = styled.section`
  width: 100%;

  &:first-child ${SectionHeader} {
    margin-top: 0;
  }
`

export const SelectAllText = styled.button`
  margin: 0;
  padding: 0;
  border: none;
  background: none;
  font: ${nala.font.small.semibold};
  color: ${nala.color.primitive.brands.rorange2};
  letter-spacing: 0.36px;
  cursor: pointer;
  flex-shrink: 0;
  opacity: 0;
  pointer-events: none;

  ${Section}:hover & {
    opacity: 1;
    pointer-events: auto;
  }

  @media (hover: none) {
    opacity: 1;
    pointer-events: auto;
  }
`

export const ScrollableColumn = styled(ScrollableCol)`
  margin-top: ${nala.spacing.l};
  padding-top: 2px;
  overflow-x: hidden;

  &::-webkit-scrollbar {
    width: 6px;
  }

  &::-webkit-scrollbar-thumb {
    background-color: ${nala.color.neutral[20]};
    border-radius: 100px;
  }

  &::-webkit-scrollbar-track {
    background-color: ${nala.color.page.background};
  }
`

export const NetworkSelectionGrid = styled.div`
  display: grid;
  /* Two columns while each item can stay on one line, then one column. */
  grid-template-columns: repeat(auto-fit, minmax(min(100%, 240px), 1fr));
  column-gap: ${nala.spacing.xl};
  row-gap: ${nala.spacing.l};
  width: 100%;
`

export const NetworkName = styled.span`
  min-width: 0;
  font: ${nala.font.default.regular};
  color: ${nala.color.text.primary};
  text-align: left;
  line-height: 20px;
`

export const NetworkSelectionContainer = styled.button`
  display: flex;
  align-items: center;
  gap: ${nala.spacing.m};
  box-sizing: border-box;
  width: 100%;
  min-width: 0;
  margin: 0;
  padding: 0;
  border: none;
  background: none;
  cursor: pointer;
  text-align: left;

  &:disabled {
    cursor: default;
    opacity: 1;
  }

  leo-checkbox {
    pointer-events: none;
    flex-shrink: 0;
    --leo-checkbox-checked-color: ${nala.color.primitive.brands.rorange2};
    --leo-checkbox-checked-color-hover: ${nala.color.primitive.brands.rorange2};
  }
`
