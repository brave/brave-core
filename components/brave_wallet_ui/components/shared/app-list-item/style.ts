// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import CheckStar from '../../../assets/svg-icons/star-checked.svg'
import UnCheckStar from '../../../assets/svg-icons/star-unchecked.svg'

export const StyledWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  border-bottom: ${(p) => `1px solid ${p.theme.color.divider01}`};
`

export const IconAndInfo = styled.div`
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: flex-start;
  flex-direction: row;
`

export const AppIconWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  min-width: 65px;
  min-height: 65px;
  background-color: ${(p) => p.theme.palette.white};
  border-radius: 6px;
  margin: 8px 8px 8px 0px;
  overflow: hidden;
`

export const AppIcon = styled.img`
  width: 65px;
  height: 65px;
`

export const AppDescColumn = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: center;
  flex-direction: column;
  width: 60%;
`

export const AppName = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  font-weight: 600;
  letter-spacing: 0.01em;
  cursor: pointer;
  color: ${(p) => p.theme.color.text01};
`

export const AppDesctription = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text02};
  word-wrap: wrap;
`

export const SelectedIcon = styled.div`
  width: 18px;
  height: 18px;
  background: url(${CheckStar});
  margin-left: 10px;
  cursor: pointer;
`

export const UnSelectedIcon = styled.div`
  width: 18px;
  height: 18px;
  background: url(${UnCheckStar});
  margin-left: 10px;
  cursor: pointer;
`
