// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const IntroImg = styled.img`
  margin-top: 16px;
  margin-bottom: 40px;
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
  margin-top: 40px;
  margin-bottom: 40px;
`

export const ButtonContainer = styled.div`
  display: flex;
  flex: 1;
  flex-direction: column;
  gap: 20px;
  & > * {
    width: 100%;
  }

  margin-bottom: 80px;
`
