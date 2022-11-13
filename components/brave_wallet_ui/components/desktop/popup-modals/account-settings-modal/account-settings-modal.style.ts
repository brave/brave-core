// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

import ClipboardIcon from '../../../../assets/svg-icons/copy-to-clipboard-icon.svg'
import { WalletButton } from '../../../shared/style'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  padding: 20px 15px 20px 15px;
  min-height: 320px;
`

export const Input = styled.input`
  outline: none;
  width: 250px;
  background-image: none;
  background-color: ${(p) => p.theme.color.background02};
  box-shadow: none;
  border: ${(p) => `1px solid ${p.theme.color.interactive08}`};
  border-radius: 4px;
  font-family: Poppins;
  font-style: normal;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  padding: 10px;
  margin-bottom: 16px;
  color: ${(p) => p.theme.color.text01};
  ::placeholder {
    font-family: Poppins;
    font-style: normal;
    font-size: 12px;
    letter-spacing: 0.01em;
    color: ${(p) => p.theme.color.text03};
    font-weight: normal;
  }
  :focus {
    outline: none;
  }
  ::-webkit-inner-spin-button {
    -webkit-appearance: none;
    margin: 0;
  }
  ::-webkit-outer-spin-button {
    -webkit-appearance: none;
    margin: 0;
  }
`

export const QRCodeWrapper = styled.img`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 210px;
  height: 210px;
  border-radius: 8px;
  border: 2px solid ${(p) => p.theme.color.disabled};
  margin-bottom: 16px;
`

export const AddressButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  font-family: Poppins;
  font-size: 16px;
  line-height: 20px;
  letter-spacing: 0.02em;
  color: ${(p) => p.theme.color.text03};
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
`

export const ButtonRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
`

export const CopyIcon = styled.div`
  width: 18px;
  height: 18px;
  background-color: ${(p) => p.theme.color.interactive07};
  -webkit-mask-image: url(${ClipboardIcon});
  mask-image: url(${ClipboardIcon});
  mask-size: cover;
  margin-left: 10px;
`

export const PrivateKeyWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  height: 100%;
`

export const WarningWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  background-color: ${(p) => p.theme.color.warningBackground};
  border-radius: 16px;
  padding: 10px;
  margin-bottom: 40px;
`

export const WarningText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  text-align: center;
  color: ${(p) => p.theme.color.text02};
  a {
    color: inherit;
    text-decoration: underline;
  }
`

export const PrivateKeyBubble = styled(WalletButton)`
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  background-color: ${(p) => p.theme.color.background01};
  padding: 5px 10px;
  max-width: 240px;
  height: auto;
  border-radius: 4px;
  margin: 0px;
  word-break: break-all;
  font-family: Poppins;
  font-size: 14px;
  line-height: 22px;
  font-weight: 600;
  color: ${(p) => p.theme.color.text01};
  outline: none;
  border: none;
`

export const ButtonWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  margin-top: 40px;
`

export const ErrorText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  color: ${(p) => p.theme.color.errorText};
  margin-bottom: 16px;
`

export const Line = styled.div`
  display: flex;
  width: 100%;
  height: 2px;
  background: ${(p) => p.theme.color.divider01};
`

export const AccountCircle = styled.div<{
  orb: string
}>`
  width: 40px;
  height: 40px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
  margin-right: 12px;
`

export const NameAndIcon = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  margin-bottom: 20px;
`

export const AccountName = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 600;
  color: ${(p) => p.theme.color.text01};
`
