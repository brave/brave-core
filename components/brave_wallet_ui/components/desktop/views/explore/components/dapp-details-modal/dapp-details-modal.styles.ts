// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'
import { WalletButton } from '../../../../../shared/style'

export const Name = styled.div`
  color: ${leo.color.text.primary};
  font-family: Poppins;
  font-size: 28px;
  font-style: normal;
  font-weight: 500;
  line-height: 40px;
  text-align: center;
`

export const CategoryTag = styled.div`
  display: flex;
  height: 20px;
  padding: 0px ${leo.spacing.s};
  align-items: center;
  border-radius: ${leo.spacing.s};
  background: ${leo.color.gray['20']};
  color: ${leo.color.gray['50']};
  font-family: Poppins;
  font-size: 10px;
  font-style: normal;
  font-weight: 600;
  line-height: normal;
  text-transform: uppercase;
`

export const Description = styled.div`
  color: ${leo.color.text.primary};
  text-align: left;
  font-family: Poppins;
  font-size: 14px;
  font-style: normal;
  font-weight: 400;
  line-height: 24px;
`

export const Stat = styled.div`
  display: flex;
  padding: ${leo.spacing.xl};
  flex-direction: column;
  align-items: center;
  gap: ${leo.spacing.s};
  flex: 1 0 0;
  border-radius: ${leo.radius.m};
  border: 1px solid ${leo.color.divider.subtle};
`

export const StatTitle = styled.div`
  color: ${leo.color.text.tertiary};
  font-family: Poppins;
  font-size: 12px;
  font-style: normal;
  font-weight: 600;
  line-height: 18px;
`

export const StatValue = styled.div`
  color: ${leo.color.text.primary};
  font-family: Poppins;
  font-size: 16px;
  font-style: normal;
  font-weight: 600;
  line-height: 24px;
`

export const NetworksTitle = styled.div`
  color: ${leo.color.text.primary};
  font-family: Poppins;
  font-size: 15px;
  font-style: normal;
  font-weight: 600;
  line-height: 20px;
  align-self: flex-start;
`

export const NetworksWrapper = styled.div`
  display: flex;
  flex-direction: row;
  gap: ${leo.spacing.m};
  padding-top: ${leo.spacing.m};
  justify-content: flex-start;
  flex-wrap: wrap;
  width: 100%;
`

export const ButtonWrapper = styled.div<{ isPanel: boolean }>`
  display: flex;
  width: 100%;
  padding-bottom: 0px;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background-color: ${(p) =>
    p.isPanel ? leo.color.container.background : 'transparent'};
  box-shadow: ${(p) =>
    p.isPanel ? '0px -8px 16px 0px rgba(0, 0, 0, 0.04)' : 'none'};
  padding: ${(p) => (p.isPanel ? '16px' : '32px')};
  border-radius: 0 0 16px 16px;
  margin-top: ${(p) => (p.isPanel ? '16px' : '0')};
`

export const VisitDappButton = styled(WalletButton)`
  display: flex;
  padding: 12px 16px;
  justify-content: center;
  align-items: center;
  gap: ${leo.spacing.m};
  border-radius: 22px;
  background: ${leo.color.button.background};
  color: ${leo.color.white};
  font-family: Poppins;
  font-size: 13px;
  font-style: normal;
  font-weight: 600;
  line-height: 20px;
  width: 100%;
  border: none;
`
export const LaunchIcon = styled(Icon).attrs({
  name: 'launch'
})`
  --leo-icon-size: 18px;
  color: ${leo.color.white};
`
