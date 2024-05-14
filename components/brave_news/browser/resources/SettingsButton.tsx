// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Button from "@brave/leo/react/button";
import { radius, spacing } from "@brave/leo/tokens/css/variables";
import styled from "styled-components";


const SettingsButton = styled(Button)`
  --leo-button-color: var(--bn-glass-50);
  --leo-button-radius: ${radius.s};
  --leo-button-padding: ${spacing.s};

  flex: 0;
`

SettingsButton.defaultProps = {
  fab: true,
  kind: 'outline'
}

export default SettingsButton
