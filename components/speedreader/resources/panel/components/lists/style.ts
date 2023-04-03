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

  button {
    display: flex;
    width: 28px;
    height: 28px;
    padding: 4px;
    justify-content: center;
    border-radius: 4px;
    border: 0;
    cursor: pointer;
    align-items: center;
    color:#6B7084;
    background-color: transparent;

    &:hover {
      color: black;
      background-color: rgba(0, 0, 0, 0.1);
    }
  }

  .sm {
    font-size: 11px;
    line-height: 1.1;
  }

  .is-active {
    color: black;
    background-color: rgba(0, 0, 0, 0.1);
  }
`
