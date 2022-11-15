// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

export const Box = styled.div`
  width: 100%;
  height: 100%;
  background: ${(p) => p.theme.color.panelBackground};
  overflow: hidden;
  min-height: 260px;
`

export const PanelContent = styled.section`
  display: flex;
  align-items: center;
  flex-direction: column;
  justify-content: center;
  padding: 20% 24px 25px 24px;
  z-index: 2;
`

export const IconBox = styled.div`
  width: 62px;
  height: 62px;
`

export const Title = styled.h1`
  color: ${(p) => p.theme.color.text01};
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 14px;
  font-weight: 400;
  letter-spacing: 0.02em;
  line-height: 2;
  margin: 0;
  text-align: center;
`

export const ButtonText = styled.button`
  --border-color: transparent;
  color: ${(p) => p.theme.color.interactive05};
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 13px;
  font-weight: 600;
  padding: 13px 4px;
  background: transparent;
  cursor: pointer;
  border: 2px solid var(--border-color);

  &:focus-visible {
    --border-color: ${(p) => p.theme.color.focusBorder};
  }
`

export const ErrorLabel = styled.div`
  margin: 6px 4px;
  color: #BD1531;
`
