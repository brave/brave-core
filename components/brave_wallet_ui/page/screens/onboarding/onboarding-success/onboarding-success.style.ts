// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// icons
import DownloadIcon from '../../../../assets/svg-icons/download-icon.svg'

export const IntroImg = styled.img`
  margin-top: 16px;
  margin-bottom: 40px;
`

export const CloseButtonContainer = styled.div`
  width: 100%;
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-end;
  margin-bottom: 44px;
`

export const IntroContainer = styled.div`
  width: 365px;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
`

export const ArticleLinksContainer = styled.div`
  width: 80%;
  display: flex;
  flex-direction: row;
  flex-wrap: wrap;
  gap: 24px;
  align-items: center;
  justify-content: center;
  margin-bottom: 60px;
`

export const ButtonContainer = styled.div`
  display: flex;
  flex: 1;
  flex-direction: column;
  gap: 24px;

  text-align: center;

  & > * {
    width: 100%;
  }

  margin-bottom: 80px;
`

export const DepositIcon = styled.div`
  cursor: pointer;
  outline: none;
  border: none;
  mask-image: url(${DownloadIcon});
  mask-position: center;
  mask-repeat: no-repeat;
  mask-size: 14px;
  background-color: ${(p) => p.theme.color.interactive05};
  height: 14px;
  width: 14px;
`
