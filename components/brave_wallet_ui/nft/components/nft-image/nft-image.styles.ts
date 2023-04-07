// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

import { WalletButton } from '../../../components/shared/style'
import MagnifyIcon from '../../../assets/svg-icons/magnify-icon.svg'

export const Image = styled.img`
  display: block;
  width: 100%;
  height: auto;
  object-fit: contain;
  border: transparent;
  border-radius: 4px;
  width: auto;
  height: 500px;
`

export const MagnifyButton = styled(WalletButton)`
  position: absolute;
  display: flex;
  align-items: center;
  justify-content: center;
  right: 17px;
  bottom: 17px;
  width: 36px;
  height: 36px;
  background-image: url(${MagnifyIcon});
  background-size: contain;
  background-color: transparent;
  cursor: pointer;
  border-radius: 50%;
  border-width: 0;
`

export const ImageWrapper = styled.div<{ isLoading: boolean }>`
  position: relative;
  display: flex;
  justify-items: center;
  align-items: center;
  visibility: ${p => p.isLoading ? 'hidden' : 'visible'};
  border-radius: 12px;
`
