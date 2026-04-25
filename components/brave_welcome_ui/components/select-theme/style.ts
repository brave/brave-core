// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const Box = styled.div`
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

  .view-note {
    font-weight: 400;
    font-size: 16px;
    margin-bottom: 48px;
  }
`

export const ThemeListBox = styled.div`
  --background-color: white;
  --text-color: #212529;
  --background-color-light: white;
  --background-color-dark: #495057;

  @media (prefers-color-scheme: dark) {
    --background-color: #1E2029;
    --text-color: white;
    --background-color-light: white;
    --background-color-dark: #495057;
  }

  .theme-list {
    display: flex;
    align-items: center;
    justify-content: center;
    margin-bottom: 56px;
    grid-gap: 15px;
    flex-wrap: wrap;

    &.is-selected-light {
      --background-color: white;
      --text-color: #212529;
      --background-color-light: white;
      --background-color-dark: #495057;
    }

    &.is-selected-dark {
      --background-color: #1E2029;
      --text-color: white;
      --background-color-light: white;
      --background-color-dark: #495057;
    }
  }

  .theme-item {
    --border-color: transparent;

    background: var(--background-color);
    border-radius: 20px;
    width: 132px;
    height: 155px;
    display: flex;
    justify-content: center;
    align-items: center;
    flex-direction: column;
    color: var(--text-color);
    border: 0;
    box-shadow: 0 0 0 4px var(--border-color);
    position: relative;

    &.is-selected {
      --border-color: #737ADE;
    }
  }

  .theme-name {
    font-weight: 400;
    font-size: 14px;
    line-height: 1.2;
    margin: 0;
    min-height: calc(2 * 14px * 1.2); // A min height of 2 lines
  }

  .logo-box {
    display: grid;
    grid-template-rows: 50px 1fr;
    gap: 10px;
    width: 100%;
  }

  .logo {
    width: 48px;
    height: 48px;
    border-radius: 48px;
    background: var(--background-color-light);
    border: 1px solid #C2C4CF;
    position: relative;
    margin: 0 auto;

    &.dark-mode {
      background: var(--background-color-dark);
    }

    &.system-mode {
      &::after {
        content: '';
        position: absolute;
        left: 0;
        width: 40px;
        height: 40px;
        border: 23px solid var(--background-color-dark);
        border-radius: 50%;
        border-bottom-color: transparent;
        border-left-color: transparent;
        transform: rotate(90deg);
      }
    }
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
`
