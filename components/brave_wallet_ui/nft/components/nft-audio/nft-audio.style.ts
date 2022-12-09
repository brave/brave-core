// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const PlaybackWrapper = styled.div`
  display: flex;
  flex-direction: column;
  width: 100%;
  height: 100%;
  position: relative;
  justify-content: center;
  align-items: center;
  border-radius: 12px;
`

export const PosterImage = styled.img`
  width: 100%;
  height: calc(100% - 54px);
  margin-bottom: 5px;
  object-fit: contain;
`

export const Audio = styled.audio`
  width: 100%;
  height: 54px;
  outline: none;
  font-size: 15px;
`
