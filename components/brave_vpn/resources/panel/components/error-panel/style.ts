// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

export const Box = styled.div`
  width: 100%;
  height: 100%;
  max-height: 450px;
  background: ${(p) => p.theme.color.panelBackground};
  position: relative;
  overflow: hidden;
`

export const PanelContent = styled.section`
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 28px 24px 16px 24px;
  position: relative;
  z-index: 2;
`

export const ReasonTitle = styled.h1`
  color: ${(p) => p.theme.color.text01};
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 18px;
  font-weight: 600;
  letter-spacing: 0.02em;
  line-height: 2.6;
  margin: 0;
  text-align: center;
`

export const ReasonDesc = styled.p`
  color: ${(p) => p.theme.color.text01};
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 14px;
  font-weight: 400;
  letter-spacing: 0.02em;
  margin: 0;
  text-align: center;
  padding-bottom: 24px;
`

export const ActionArea = styled.div`
  width: 100%;
  display: grid;

  button {
    &:first-child {
      margin-bottom: 10px;
    }
  }
`

export const PanelHeader = styled.section`
  display: flex;
  align-items: center;
  width: 100%;
  margin-bottom: 10px;
  box-sizing: border-box;
`

export const BackButton = styled.button`
  background-color: transparent;
  border: 0;
  padding: 0;
  display: flex;
  align-items: center;
  cursor: pointer;

  span {
    font-family: ${(p) => p.theme.fontFamily.heading};
    font-size: 13px;
    font-weight: 600;
    color: ${(p) => p.theme.color.interactive05};
  }

  i {
    width: 18px;
    height: 18px;
    margin-right: 5px;
  }

  svg {
    fill: ${(p) => p.theme.color.interactive05};
  }
`
