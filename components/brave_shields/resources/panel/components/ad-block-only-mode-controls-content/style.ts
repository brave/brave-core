// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const FooterActionBox = styled.div`
  padding: 0 22px 17px 22px;
  display: grid;
  grid-template-columns: 14px 1fr;
  grid-gap: 10px;

  div {
    grid-column: 2;
    display: flex;
    flex-direction: column;
    grid-gap: 20px;
  }

  button {
    --svg-color: ${(p) => p.theme.color.interactive05};
    --text-color: ${(p) => p.theme.color.interactive06};
    background-color: transparent;
    padding: 0;
    margin: 0;
    border:0;
    color: var(--text-color);
    font-size: 13px;
    font-weight: 500;
    text-decoration: none;
    display: flex;
    align-items: center;
    cursor: pointer;

    svg > path {
      fill: var(--svg-color);
    }

    &:hover {
      --text-color: ${(p) => p.theme.color.interactive07};
      --svg-color: ${(p) => p.theme.color.interactive08};
    }
  }

  i {
    display: block;
    width: 17px;
    height: 17px;
    margin: 0 6px;
  }
`
