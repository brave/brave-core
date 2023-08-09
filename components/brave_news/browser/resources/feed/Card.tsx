// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import styled from "styled-components";
import { color, effect, radius, spacing } from '@brave/leo/tokens/css'

export default styled.div`
  background: ${color.container.background};
  box-shadow: ${effect.elevation[7]};
  border-radius: ${radius[8]};
  border: 1px solid ${color.gray[20]};
  color: ${color.text.primary};
  padding: ${spacing[8]};
`
