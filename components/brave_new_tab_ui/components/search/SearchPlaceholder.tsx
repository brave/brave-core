// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react';
import SearchBox from './SearchBox';
import SearchDialog from './SearchDialog';
import { useNewTabPref } from '../../hooks/usePref';
import { SearchContext } from './SearchContext';

export default function SearchPlaceholder() {
  const [open, setOpen] = React.useState(false)
  const [boxPos, setBoxPos] = React.useState(0)
  const [showSearchBox] = useNewTabPref('showSearchBox')
  if (!showSearchBox) return null
  return <SearchContext>
    {!open && <div onClick={e => {
      setOpen(true)
      setBoxPos(e.currentTarget.getBoundingClientRect().y)
    }}>
      <SearchBox />
    </div>}
    {open && <SearchDialog offsetY={boxPos} onClose={() => setOpen(false)} />}
  </SearchContext>
}
