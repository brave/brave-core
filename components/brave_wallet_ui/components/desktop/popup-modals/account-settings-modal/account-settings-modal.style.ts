// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import AlertReact from '@brave/leo/react/alert'
import * as leo from '@brave/leo/tokens/css/variables'
import LeoSegmentedControl, {
  SegmentedControlProps
} from '@brave/leo/react/segmentedControl'

// Assets
import ClipboardIcon from '../../../../assets/svg-icons/copy-to-clipboard-icon.svg'

// Shared Styles
import { WalletButton, Row } from '../../../shared/style'
import {
  layoutPanelWidth //
} from '../../wallet-page-wrapper/wallet-page-wrapper.style'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 20px 0px;
  height: 100%;
  min-height: 200px;
`

export const EditWrapper = styled(StyledWrapper)`
  gap: 14px;
  & > * {
    width: 250px;
  }
`

export const QRCodeWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 185px;
  height: 185px;
  border-radius: 16px;
  border: 2px solid ${leo.color.divider.subtle};
  margin-bottom: 16px;
`

export const QRCodeImage = styled.img`
  width: 170px;
  height: 170px;
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
  word-wrap: break-word;
  word-break: break-all;
  flex-wrap: wrap;
`

export const ButtonRow = styled.div`
  width: 100%;
  display: flex;
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
  width: 250px;
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
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  margin-top: 40px;

  & > * {
    width: 100%;
  }
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

export const Alert = styled(AlertReact)`
  margin-bottom: 15px;

  &:first-of-type {
    margin: 15px 0px;
  }
`

export const ControlsWrapper = styled(Row)`
  margin-bottom: 24px;
  --leo-segmented-control-width: 100%;
`

export const SegmentedControl = styled(LeoSegmentedControl).attrs({
  size: window.innerWidth <= layoutPanelWidth ? 'small' : 'default'
})<SegmentedControlProps>``
