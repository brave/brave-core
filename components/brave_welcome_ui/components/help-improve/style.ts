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
    padding: 40px 40px 50px 40px;
  }

  .view-details {
    grid-column: 2;
  }

  .view-title {
    font-weight: 600;
    font-size: 36px;
    margin: 0;
  }
`

export const Grid = styled.div`
  font-size: 16px;
  font-weight: 400;
  margin-bottom: 55px;
  display: grid;
  grid-template-columns: 0.5fr 1fr 0.5fr;

  .list {
    grid-column: 2;
  }

  .item {
    display: grid;
    align-items: flex-start;
    grid-template-columns: 26px 1fr;
    gap: 10px;
    margin-bottom: 20px;
    line-height: 20px;

    a {
      text-decoration: none;
      color: rgba(160, 165, 235, 1);
    }
  }

  input[type="checkbox"] {
    width: 22px;
    height: 22px;
    border-radius: 4px;
    border: 1px solid #AEB1C2;

    &:checked {
      accent-color: #4C54D2;
    }
  }
`

export const ActionBox = styled.div`
  width: 100%;
  display: grid;
  grid-template-columns: 0.5fr 1fr 0.5fr;
  margin-bottom: 40px;

  .box-center {
    grid-column: 2;
  }

  button:first-child {
    width: 100%;
  }
`

export const FootNote = styled.div`
  font-size: 12px;
  margin-top: 40px;
  line-height: 18px;

  a, button {
    color: inherit;
  }

  span {
    margin-left: 5px;
  }
`
