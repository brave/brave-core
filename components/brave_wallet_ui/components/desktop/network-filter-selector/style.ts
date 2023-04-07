// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { WalletButton } from '../../shared/style'
import { CaratStrongDownIcon } from 'brave-ui/components/icons'
import CheckMark from '../../../assets/svg-icons/big-checkmark.svg'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  position: relative;
`

export const DropDownButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  background-color: ${(p) => p.theme.color.background02};
  cursor: pointer;
  outline: none;
  background: none;
  border: ${(p) => `1px solid ${p.theme.color.interactive08}`};
  border-radius: 4px;
  font-family: Poppins;
  font-style: normal;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  padding: 8px 12px;
  margin-bottom: 8px;
  color: ${(p) => p.theme.color.text01};
`

export const DropDownIcon = styled(CaratStrongDownIcon)`
  width: 18px;
  height: 18px;
  color: ${(p) => p.theme.color.interactive07};
`

export const DropDown = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  width: 250px;
  padding: 5px;
  background-color: ${(p) => p.theme.color.background02};
  border: 1px solid ${(p) => p.theme.color.divider01};
  border-radius: 8px;
  box-shadow: 0px 0px 16px rgba(99, 105, 110, 0.18);
  @media (prefers-color-scheme: dark) {
    box-shadow: 0px 0px 16px rgba(0, 0, 0, 0.36);
  }
  position: absolute;
  top: 38px;
  z-index: 9;
  @media screen and (max-width: 800px) {
    right: 0px;
  }
`

export const NetworkItemWrapper = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  position: relative;
  width: 100%;
`

export const NetworkItemButton = styled(WalletButton)`
  display: flex;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  margin: 0px;
  padding: 8px 8px 8px 12px;
  min-height: 40px;
  box-sizing: border-box;
  border-radius: 6px;
  &:hover {
    background-color: ${(p) => p.theme.color.divider01};
  }
`

export const LeftSide = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  flex-direction: row;
  max-width: 90%;
`

export const SelectorLeftSide = styled(LeftSide)`
  white-space: nowrap;
  margin-right: 4px;
`

export const NetworkName = styled.span`
  font-family: Poppins;
  font-size: 14px;
  letter-spacing: 0.01em;
  font-weight: 400;
  color: ${(p) => p.theme.color.text01};
  text-align: left;
`

export const BigCheckMark = styled.div`
  width: 14px;
  height: 14px;
  background-color: ${(p) => p.theme.color.text01};
  -webkit-mask-image: url(${CheckMark});
  mask-image: url(${CheckMark});
  margin-right: 8px;
`

export const SecondaryNetworkText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  letter-spacing: 0.01em;
  font-weight: 400;
  color: ${(p) => p.theme.color.text03};
  text-align: left;
  margin: 10px 0px 10px 10px;
`

export const ClickAwayArea = styled.div`
  left: 0;
  right: 0;
  top: 0;
  bottom: 0;
  position: fixed;
  z-index: 7;
`
