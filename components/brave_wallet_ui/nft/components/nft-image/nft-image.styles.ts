// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

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
  width: ${(props) => props.customWidth || 'auto'};
  height: ${(props) => props.customHeight || 'auto'};
`

export const ImageWrapper = styled.div`
  position: relative;
  display: flex;
  justify-items: center;
  align-items: center;
  border-radius: 12px;
`
