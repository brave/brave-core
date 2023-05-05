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
  background-color: transparent;
  gap: 8px;

  .chip {
    display: grid;
    grid-template-columns: 1fr;

    width: 20px;
    height: 20px;
    border-radius: 100px;
    border: 1px solid var(--color-border);
    cursor: pointer;
    padding: 0;
    overflow: hidden;
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

  .mark {
    grid-area: 1 / 1 / 2 / 2;
    width: 16px;
    height: 16px;
    align-self: center;
    justify-self: center;
    color: #5F5CF1;
  }

  .is-light { background-color: #FFFFFF; }
  .is-dark { background-color: #000000; }
  .is-sepia { background-color: #F2EBD9; }
`
