// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

export const Box = styled.div`
  --speedreader-background: #FFFFFF;
  background: var(--speedreader-background);
  color: ${(p) => p.theme.color.text01};
  width: 100%;
  height: 100%;
  font-family: ${(p) => p.theme.fontFamily.heading};
  flex: 1 1 auto;
  position: relative;

  @media (prefers-color-scheme: dark) {
    --speedreader-background: #1E2029;
  }

  a {
    color: ${(p) => p.theme.color.interactive05};
  }
`

export const HeaderBox = styled.div`
  border-bottom: 1px solid #3B3E4F;
  margin-bottom: 24px;
`

export const HeaderContent = styled.div`
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 16px 24px;

  div:first-child {
    flex: 1 1 auto;
    font-weight: 500;
    font-size: 20px;
    line-height: 20px;
  }
`

export const Section = styled.section`
  margin-bottom: 16px;
  padding: 0 24px;
  line-height: 18px;
  font-weight: 400;

  .title {
    color: ${(p) => p.theme.color.text02};
    font-style: normal;
    font-weight: 600;
    font-size: 14px;
    margin-bottom: 10px;
  }
`

export const SiteName = styled.div`
  font-weight: 600;
`

export const Bg = styled.div`
  position: absolute;
  bottom: 0;
  width: 100%;
  padding: 0 24px 5px 24px;
  color: #F1F3F5;
  opacity: 0.1;
  pointer-events: none;
`

export const SiteTitleBox = styled.div`
  padding: 0 24px 24px 24px;
`
