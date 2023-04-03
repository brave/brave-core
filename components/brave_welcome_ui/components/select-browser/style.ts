// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const MainBox = styled.div`
  background: rgba(255, 255, 255, 0.1);
  backdrop-filter: blur(15px);
  border-radius: 30px;
  max-width: 800px;
  color: white;
  font-family: ${(p) => p.theme.fontFamily.heading};
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;

  .view-header-box {
    display: grid;
    grid-template-columns: 0.2fr 1fr 0.2fr;
    padding: 40px;
    margin-bottom: 50px;
    text-align: center;
  }

  .view-details {
    grid-column: 2;
  }

  .view-title {
    font-weight: 600;
    font-size: 36px;
    margin: 0 0 18px 0;
  }

  .view-desc {
    font-weight: 400;
    font-size: 16px;
    margin: 0;
  }
`

export const BrowserListBox = styled.div`
  .browser-list {
    display: flex;
    align-items: center;
    justify-content: center;
    margin-bottom: 56px;
    grid-gap: 15px;
    flex-wrap: wrap;
  }

  .browser-item {
    --border-color: transparent;
    background: white;
    border-radius: 20px;
    width: 132px;
    height: 155px;
    display: flex;
    justify-content: center;
    align-items: center;
    flex-direction: column;
    color: #212529;
    border: 0;
    box-shadow: 0 0 0 4px var(--border-color);
    position: relative;

    &.is-selected {
      --border-color: #737ADE;
    }
  }

  .browser-name {
    font-weight: 400;
    font-size: 14px;
    margin: 0;
  }

  .browser-logo-box {
    width: 60px;
    height: 60px;
    margin-bottom: 10px;
  }

  .check-icon-box {
    width: 16px;
    height: 16px;
    position: absolute;
    top: 14px;
    right: 14px;
  }
`

export const ActionBox = styled.div`
  display: flex;
  grid-gap: 10px;
  margin-bottom: 40px;

  button {
    color: white;
  }

  button:nth-child(2) {
    &[disabled] {
      background: rgba(255, 255, 255, 0.14);
      backdrop-filter: blur(8px);
      color: rgba(255, 255, 255, 0.32);
    }
  }
`
