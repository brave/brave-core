// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

export const Box = styled.div`
  display: grid;
  grid-template-columns: 1fr 1fr 1fr;
  grid-gap: 6px;

  .chip {
    margin: 0 auto;
    width: 20px;
    height: 20px;
    background: transparent;
    border-radius: 100px;
    border: 1px solid #E6EBEF;
    position: relative;
    padding: 0;
    cursor: pointer;
  }

  .icon-box {
    width: 100%;
    height: 100%;
    background: transparent;
    border-radius: 8px;
    overflow: hidden;
  }
`
