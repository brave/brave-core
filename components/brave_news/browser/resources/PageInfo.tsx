// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Dropdown from '@brave/leo/react/dropdown';
import * as React from 'react';
import { pages, useInspectContext } from './context';

interface Props {
}

export default function PageInfo(props: Props) {
  const { page, setPage } = useInspectContext();

  return <div>
    <div>
      <Dropdown value={page} onChange={e => setPage(e.detail.value)}>
        <span slot="label">Page</span>
        {pages.map(p => <leo-option key={p}>{p}</leo-option>)}
      </Dropdown>
    </div>
  </div>
}
