// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

export const modalSize = '640px'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  left: 0;
  right: 0;
  top: 0;
  bottom: 0;
  position: fixed;
  z-index: 10;
  background: rgba(33, 37, 41, 0.8);
`

export const Modal = styled.div<{ width?: string, height?: string }>`
  position: relative;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-conent: center;
  min-width: ${p => p.width ? p.width : '580px'};
  max-width: ${p => p.width ? p.width : '580px'};
  min-height: ${p => p.height ? p.height : '580px'};
  max-height: ${p => p.height ? p.height : '580px'};
  background-color: transparent;
  border-radius: 8px;
  @media screen and (max-width: 600px) {
    min-width: 480px;
    max-width: 480px;
    min-height: 480px;
    max-height: 480px;
  }
`
