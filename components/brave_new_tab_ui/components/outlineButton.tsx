// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

const Button = styled('button')`
  --outer-border-size: 0px;
  --inner-border-size: 1px;
  appearance: none;
  cursor: pointer;
  background: none;
  border-radius: 24px;
  border: solid var(--border-size);
  padding: 10px 22px;
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  gap: 4px;
  color: white;
  font: 600 13px/20px ${p => p.theme.fontFamily.heading};
  /* Use box-shadow for borders so we get a smooth (and layout-free performance-cheap)
    animation as well as multiple borders. */
  box-shadow: 0 0 0 var(--outer-border-size) rgba(255, 255, 255, .6),
              inset 0 0 0 var(--inner-border-size) white;
  transition: box-shadow .12s ease-in-out;
  outline: none;

  :focus {
    outline: none;
  }

  :hover {
    --inner-border-size: 2px;
  }

  :focus-visible {
    --inner-border-size: 1px;
    --outer-border-size: 2px;
  }

  :active {
    --inner-border-size: 1px;
    --outer-border-size: 4px;
  }
`

export const LinkButton = styled(Button.withComponent('a'))`
  text-decoration: none;
  :hover {
    text-decoration: none;
  }
`

export default Button
