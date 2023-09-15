// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const Box = styled.div`
  display: flex;
  height: 100%;
  flex-direction: row;
  justify-content: center;
  align-items: center;

  #theme-system .icon-box {
    color: black;
  }

  .chip {
    display: grid;
    grid-template-columns: 1fr;

    width: 20px;
    height: 20px;
    border-radius: var(--leo-radius-full);
    border: 1px solid var(--color-button-border);
    cursor: pointer;
    padding: 0;
    overflow: hidden;
    margin-left: 8px;
    justify-content: center;
    align-items: center;
  }

  .icon-box {
    grid-area: 1 / 1 / 2 / 2;

    display: flex;
    align-items: center;
    justify-content: center;

    width: 100%;
    height: 100%;
    background: transparent;
    overflow: hidden;
    border: none;
  }

  .system-theme-icon {
    --leo-icon-size: 24px;
    width: var(--leo-icon-size);
  }

  .mark {
    grid-area: 1 / 1 / 2 / 2;
    align-self: center;
    justify-self: center;
    color: #5F5CF1;
    position: relative;

    &:before {
      content: '';
      position: absolute;
      width: 8px;
      height: 8px;
      left: 4px;
      top: 4px;
      border-radius: 100px;
      background-color: white;
    }
  }

  .is-light { background-color: #FFFFFF; }
  .is-dark { background-color: #000000; }
  .is-sepia { background-color: #F2EBD9; }
`
