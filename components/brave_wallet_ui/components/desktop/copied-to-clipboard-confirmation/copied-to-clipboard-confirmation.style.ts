// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const CopiedToClipboardContainer = styled.div`
  align-self: center;
  justify-self: center;
  text-align: center;
  display: flex;
  align-items: center;
  justify-content: center;
  width: 100%;
  font-size: 12px;
  font-weight: 500;

  & p {
    margin-left: 4px;
    font-family: 'Poppins';
    font-style: normal;
    font-weight: 500;
    font-size: 12px;
    line-height: 20px;
    text-align: center;
    color: ${(p) => p.theme.color.text01};
  }
`
