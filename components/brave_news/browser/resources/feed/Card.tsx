// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { color, effect, font, radius, spacing } from '@brave/leo/tokens/css';
import styled from "styled-components";

export const Header = styled.h2`
  margin: 0;

  font: ${font.primary.heading.h2};
  color: var(--bn-glass-100);

  --leo-icon-size: 18px;
`

export const Title = styled.h3`
  --leo-icon-color: ${color.icon.default};

  display: flex;

  margin: 0;

  font: ${font.primary.default.regular};
  color: var(--bn-glass-100);


  &> a { all: unset; }
`

export const SmallImage = styled.img`
  &:not([src]) { opacity: 0; }

  min-width: 96px;
  width: 96px;

  height: 64px;

  object-fit: cover;
  object-position: top;

  border-radius: 6px;
`

export const LargeImage = styled.img`
  &:not([src]) { opacity: 0; }

  width: 100%;
  height: 269px;

  object-fit: cover;
  object-position: top;

  border-radius: 6px;
`

export default styled.div`
  text-decoration: none;
  background: var(--bn-glass-container);
  border-radius: ${radius.xl};
  color: var(--bn-glass-100);
  padding: ${spacing["2Xl"]};

  &:has(${Title} a:focus-visible) {
    box-shadow: ${effect.focusState};
  }

  ${p => p.onClick && 'cursor: pointer'}
`
