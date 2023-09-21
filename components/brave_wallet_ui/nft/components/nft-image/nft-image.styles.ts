// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

export const Image = styled.img`
  display: block;
  width: 100%;
  height: auto;
  object-fit: contain;
  border: transparent;
  border-radius: 8px;
  width: auto;
  height: 360px;
`

export const ImageWrapper = styled.div`
  position: relative;
  display: flex;
  justify-items: center;
  align-items: center;
  border-radius: 12px;
`
