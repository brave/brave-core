// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const Box = styled.div`
  position: relative;
  max-width: 800px;
  border-radius: 30px;
  color: white;
  font-family: ${(p) => p.theme.fontFamily.heading};
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  text-align: center;

  .view-backdrop {
    background: rgba(255, 255, 255, 0.1);
    backdrop-filter: blur(15px);
    border-radius: 30px;
    position: absolute;
    top: 0;
    z-index: 1;
    width: 100%;
    height: 100%;

    &.initial {
      scale: 0.9;
      opacity: 0;
    }
  }

  .view-header-box {
    display: grid;
    grid-template-columns: 0.2fr 1.5fr 0.2fr;
    padding: 100px 40px 50px 40px;
  }

  .view-content {
    position: relative;
    z-index: 3;

    // We define an initial state here if animations are enabled
    &.initial {
      transform: translateY(20px);
      opacity: 0;
    }
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

  .view-logo-box {
    width: 150px;
    height: auto;
    position: absolute;
    top: calc(-160px / 2);
    left: calc(50% - 160px/2);
    z-index: 2;

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
  padding: 0 40px;
  max-width: 450px;
  margin: 0 auto 40px auto;

  button {
    color: white;
  }
`
