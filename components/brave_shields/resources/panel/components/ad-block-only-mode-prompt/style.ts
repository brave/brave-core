// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Alert from '@brave/leo/react/alert'
import styled from 'styled-components'

export const Container = styled.div`
  padding: var(--leo-spacing-xl);
`

export const Title = styled.div`
  font: var(--leo-font-heading-h4);
`

export const Description = styled.div`
  font: var(--leo-font-default-regular);
  a {
    color: var(--leo-color-text-secondary);
  }
`

export const Actions = styled.div`
  padding-top: var(--leo-spacing-xl);
  display: flex;
  gap: var(--leo-spacing-m);
`

export const StyledAlert = styled(Alert)`
  padding: 0 var(--leo-spacing-xl) var(--leo-spacing-xl);
`

export const ActionsSlotWrapper = styled.div`
  display: flex;
  gap: var(--leo-spacing-m);
`
