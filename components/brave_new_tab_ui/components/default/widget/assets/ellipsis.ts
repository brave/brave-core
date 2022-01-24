// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import EllipsisIcon from '../../../popupMenu/ellipsisIcon'

interface Props {
  lightWidget?: boolean
}

export default styled(EllipsisIcon)<Props>`
  color: ${p => p.lightWidget ? '#495057' : '#ffffff'};
`
