// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const MainBox = styled.div`
  --common-gap: 28px;

  background: rgba(255, 255, 255, 0.1);
  backdrop-filter: blur(15px);
  border-radius: 30px;
  max-width: 656px;
  color: white;
  font-family: ${(p) => p.theme.fontFamily.heading};
  display: flex;
  gap: var(--common-gap);
  align-items: center;
  justify-content: center;
  flex-direction: column;
  padding: 70px 109px 64px 109px;
  margin-top: 80px;

  .view-header-box {
    display: flex;
    gap: 19px;
    flex-direction: column;
    text-align: center;
  }

  .view-logo-box {
    --width: 194px;
    --height: 181px;

    width: var(--width);
    height: var(--height);
    position: absolute;
    left: calc(50% - var(--width) / 2);
    top: calc(-1 * var(--height) + 52px);
    z-index: 2;
  }

  .view-title {
    font-weight: 600;
    font-size: 36px;
    margin: 0;
  }

  .view-subtitle {
    margin: 0;
    font-size: 20px;
    font-style: normal;
    font-weight: 600;
    line-height: 30px;
    max-width: 400px;
  }
`

export const BodyBox = styled.div`
  font-family: Poppins;
  font-size: 14px;
  font-style: normal;
  font-weight: 400;
  line-height: 20px;

  a {
    text-decoration: underline;
    color: currentColor;
    font-size: 13px;
    line-height: 24px;
    letter-spacing: 0.13px;
  }
`

export const ActionBox = styled.div`
  width: 100%;
  max-width: 368px;
  display: flex;
  gap: var(--common-gap);
  flex-direction: column;
  font-size: 16px;
  font-weight: 600;

  button {
    color: white;
  }
`
