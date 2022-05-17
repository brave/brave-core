// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

import { WalletButton } from '../../../../components/shared/style'
import ClipboardIcon from '../../../../assets/svg-icons/clipboard-icon.svg'
import DownloadIcon from '../../../../assets/svg-icons/download-icon.svg'

export const LinkText = styled.a`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 14px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.interactive05};
  margin: 0px;
`

// Phrase card
export const PhraseCard = styled.div`
  display: flex;
  flex: 1;
  flex-direction: column;
  width: 376px;
  align-items: center;
`

export const PhraseCardTopRow = styled.div`
  display: flex;
  flex-direction: row;
  width: 375px;
  height: 40px;
  align-items: center;
  justify-content: flex-end;
  padding: 14px 8px;
`

export const PhraseCardBody = styled.div`
  flex: 1;
  width: 100%;
  border-style: solid;
  border-width: 1px;
  border-color: ${(p) => p.theme.color.divider01};
  border-radius: 4px;
`

export const PhraseCardBottomRow = styled(PhraseCardTopRow)`
  justify-content: flex-start;
  height: 40px;
  gap: 14px;
`

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
