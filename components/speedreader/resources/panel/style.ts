// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

export const ToolbarWrapperStyles = styled.div`
  width: 100%;
  height: 2.5rem;
  display: flex;
  background: var(--color-background);
  color: var(--color-foreground);
  font-family: ${(p) => p.theme.fontFamily.heading};
  border-bottom: 1px solid var(--color-border);
  overflow: hidden;

  --leo-icon-size: 16px;
`
