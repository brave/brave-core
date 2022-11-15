// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

import { LoaderIcon } from 'brave-ui/components/icons'
import { WalletButton } from '../../../shared/style'

export const FooterContainer = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  margin-top: 12px;
`

export const QueueStepRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-end;
  flex-direction: row;
`

export const QueueStepText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  color: ${p => p.theme.color.text02};
  font-weight: 600;
  margin-right: 9px;
`

export const QueueStepButton = styled(WalletButton)<{ needsMargin?: boolean }>`
  font-family: Poppins;
  font-style: normal;
  font-weight: 600;
  font-size: 13px;
  color: ${p => p.theme.color.interactive05};
  background: none;
  cursor: pointer;
  outline: none;
  border: none;
  margin: 0;
  padding: 0;
  margin-bottom: ${p => (p.needsMargin ? '12px' : '0px')};
`

export const ErrorText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${p => p.theme.color.errorText};
  text-align: center;
`

export const ConfirmingButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: default;
  border-radius: 40px;
  padding: 8px 16px;
  outline: none;
  margin: 0px;
  background-color: ${p => p.theme.color.disabled};
  border: none;
`

export const ConfirmingButtonText = styled.span`
  font-style: normal;
  font-weight: 600;
  font-size: 13px;
  line-height: 20px;
  text-align: center;
  color: ${p => p.theme.color.interactive07};
  flex: none;
  order: 1;
  flex-grow: 0;
  margin: 0px 8px;
`

export const LoadIcon = styled(LoaderIcon)`
  color: ${p => p.theme.color.interactive08};
  height: 25px;
  width: 24px;
  opacity: 0.4;
`

export const ButtonRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  width: 100%;
  margin-bottom: 14px;
  margin-top: 12px;
`

export const FavIcon = styled.img`
  width: auto;
  height: 40px;
  border-radius: 5px;
  background-color: ${p => p.theme.color.background01};
  margin-bottom: 7px;
`
