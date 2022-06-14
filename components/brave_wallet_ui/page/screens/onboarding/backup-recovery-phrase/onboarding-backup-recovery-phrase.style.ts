// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

import { WalletButton } from '../../../../components/shared/style'
import ClipboardIcon from '../../../../assets/svg-icons/clipboard-icon.svg'
import DownloadIcon from '../../../../assets/svg-icons/download-icon.svg'

// buttons
export const CopyButton = styled(WalletButton)`
  cursor: pointer;
  outline: none;
  border: none;
  mask-image: url(${ClipboardIcon});
  mask-position: center;
  mask-repeat: no-repeat;
  mask-size: 14px;
  background-color: ${(p) => p.theme.color.text01};
  height: 14px;
  width: 14px;
`

export const DownloadButton = styled(WalletButton)`
  cursor: pointer;
  outline: none;
  border: none;
  mask-image: url(${DownloadIcon});
  mask-position: center;
  mask-repeat: no-repeat;
  mask-size: 14px;
  background-color: ${(p) => p.theme.color.text01};
  height: 14px;
  width: 14px;
`

export const CopiedToClipboardContainer = styled.div`
  align-self: center;
  justify-self: center;
  text-align: center;
  display: flex;
  align-items: center;
  justify-content: center;
  width: 100%;
  padding-right: 55px;
  font-size: 12px;
  font-weight: 500;

  & p {
    margin-left: 4px;
    font-family: 'Poppins';
    font-style: normal;
    font-weight: 500;
    font-size: 12px;
    line-height: 20px;
    text-align: center;
    color: ${(p) => p.theme.color.text01};
  }
`
