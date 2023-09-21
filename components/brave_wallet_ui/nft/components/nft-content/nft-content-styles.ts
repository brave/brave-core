// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const ImageWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  width: 100%;
  height: 100%;
`

export const MultimediaWrapper = styled.div`
  position: relative;
  display: flex;
  align-items: center;
  width: 100%;
  height: auto;
  overflow: hidden;
`

export const Image = styled.img<{
  customWidth?: string
  customHeight?: string
}>`
  display: block;
  border: transparent;
  border-radius: 8px;
  max-width: 100%;
  max-height: 100%;
  position: relative;
  object-fit: contain;
`
