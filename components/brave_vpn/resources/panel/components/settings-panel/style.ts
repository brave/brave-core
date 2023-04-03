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
`

export const List = styled.ul`
  list-style-type: none;
  padding: 0;
  margin: 8px;
  text-align: left;

  li {
    margin-bottom: 28px;

    &:last-child {
      margin-bottom: 0;
    }
  }

  a {
    font-family: ${(p) => p.theme.fontFamily.heading};
    font-size: 13px;
    font-weight: 600;
    color: ${(p) => p.theme.color.interactive05};
    text-decoration: none;
  }
`

export const Card = styled.li`
  --divider-color: ${(p) => p.theme.color.divider01};
  border: 1px solid var(--divider-color);
  border-radius: 8px;
`

export const PanelContent = styled.section`
  padding: 25px 24px 25px 24px;
  z-index: 2;
`

export const PanelHeader = styled.section`
  display: flex;
  align-items: center;
  width: 100%;
  margin-bottom: 18px;
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

export const ReconnectBox = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  width: 100%;

  span {
    font-family: ${(p) => p.theme.fontFamily.heading};
    font-size: 13px;
    font-weight: 600;
    color: ${(p) => p.theme.color.interactive05};
  }
`
