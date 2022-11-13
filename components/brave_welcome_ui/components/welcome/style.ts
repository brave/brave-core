// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

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
  text-align: center;
  position: relative;

  .view-header-box {
    display: grid;
    grid-template-columns: 0.2fr 1.5fr 0.2fr;
    padding: 100px 40px 50px 40px;
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
    font-size: 20px;
    margin: 0;
  }

  .brave-logo-box {
    width: 150px;
    height: 150px;
    position: absolute;
    top: calc(-160px / 2);

    img {
      width: 100%;
      height: auto;
    }
  }
`

export const ActionBox = styled.div`
  display: flex;
  flex-direction: column;
  grid-gap: 10px;
  margin-bottom: 40px;

  button:nth-child(2) {
    box-shadow: none;
  }
`
