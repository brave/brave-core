// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import Button from '@brave/leo/react/button'
import * as leo from '@brave/leo/tokens/css/variables'

export const CancelButton = styled(Button)`
  --leo-button-color: ${leo.color.button.errorBackground};
`
