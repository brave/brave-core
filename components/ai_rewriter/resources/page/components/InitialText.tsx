// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react';
import { useRewriterContext } from '../Context';
import Input from '@brave/leo/react/input'
import styled from 'styled-components';

const StyledInput = styled(Input)`
  width: 100%;
`

export default function InitialText() {
  const { initialText, setInitialText } = useRewriterContext()
  return <StyledInput placeholder="Enter text to rewrite" value={initialText} onChange={({ value }) => setInitialText(value)}>
    Initial text
  </StyledInput>
}
