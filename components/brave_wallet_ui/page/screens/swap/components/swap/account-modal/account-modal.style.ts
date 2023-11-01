// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { StyledDiv, Text, StyledButton, Icon } from '../../shared-swap.styles'

export const ModalBox = styled(StyledDiv)`
  background-color: ${(p) => p.theme.color.background01};
  min-width: 280px;
  position: absolute;
  padding-bottom: 4px;
  z-index: 10;
  top: 42px;
  box-shadow: 0px 0px 16px rgba(99, 105, 110, 0.18);
  right: 0px;
  border-radius: 16px;
  white-space: nowrap;
  @media (prefers-color-scheme: dark) {
    box-shadow: 0px 0px 16px rgba(0, 0, 0, 0.36);
  }
  @media screen and (max-width: 570px) {
    position: fixed;
    right: 0px;
    left: 0px;
    bottom: 0px;
    top: unset;
    width: auto;
    height: auto;
    border-radius: 16px 16px 0px 0px;
  }
`

export const Title = styled(Text)`
  @media screen and (max-width: 570px) {
    font-weight: 600;
  }
`

export const ModalButton = styled(StyledButton)`
  display: flex;
  justify-content: flex-start;
  font-weight: 500;
  font-size: 14px;
  color: ${(p) => p.theme.color.text02};
  width: 100%;
  margin: 0px;
  margin-right: 16px;
  padding: 12px 4px;
`

export const ModalButtonIcon = styled(Icon)`
  color: ${(p) => p.theme.color.text02};
  margin-right: 10px;
`

export const AccountButton = styled(StyledButton)`
  --account-text-color: ${(p) => p.theme.color.text02};
  display: flex;
  justify-content: flex-start;
  background-color: ${(p) => p.theme.color.background01};
  padding: 8px 10px;
  width: 100%;
  &:hover {
    --account-text-color: ${(p) => p.theme.color.text01};
  }
`

export const AccountCircle = styled(StyledDiv)<{ orb: string }>`
  width: 24px;
  height: 24px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
  margin-right: 8px;
  @media screen and (max-width: 570px) {
    width: 40px;
    height: 40px;
    margin-right: 16px;
  }
`

export const AccountText = styled(Text)`
  color: var(--account-text-color);
`
