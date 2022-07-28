// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const NftImageIframe = styled.iframe`
  border: none;
  width: ${p => p.width ? p.width : '40px'};
  height: ${p => p.height ? p.height : '40px'};
`
