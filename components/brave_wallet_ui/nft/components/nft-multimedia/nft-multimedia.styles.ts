// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const MultiMediaWrapper = styled.div`
  position: relative;
  display: flex;
  align-items: center;
  justify-content: center;
`

export const NftMediaIframe = styled.iframe<{ visible: boolean }>`
  width: ${p => p.visible ? '100%' : '0'};
  min-width: ${p => p.visible ? '100%' : '0'};
  min-height: 100vh;
  border: none;
  visibility: ${p => p.visible ? 'visible' : 'hidden'};
  border-radius: 12px;
`

export const MediaSkeletonWrapper = styled.div`
  width: 75%;
  height: 500px;
  border-radius: 12px;
`
