// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import styled from "styled-components";
import Card from "./Card";
import ProgressRing from '@brave/leo/react/progressRing'
import * as React from "react";

const Container = styled(Card)`
  display: flex;
  align-items: center;
  justify-content: center;
`

const Spinner = styled(ProgressRing)`
  --leo-progressring-color: var(--bn-glass-25);
`

export default function LoadingCard() {
  return <Container>
    <Spinner />
  </Container>
}
