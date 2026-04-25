// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import Button from '@brave/leo/react/button'
import styled from 'styled-components'

export default styled(Button)`
  --leo-button-color: var(--bn-glass-10);

  border-radius: 20px;
  overflow: hidden;
  backdrop-filter: brightness(0.8) blur(32px);
`
