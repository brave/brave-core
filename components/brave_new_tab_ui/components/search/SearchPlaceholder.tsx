// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react';
import SearchBox, { Backdrop } from './SearchBox';
import SearchDialog from './SearchDialog';
import { useNewTabPref } from '../../hooks/usePref';
import { SearchContext, useSearchContext } from './SearchContext';
import styled from 'styled-components';
import { radius } from '@brave/leo/tokens/css/variables';

const PlaceholderContainer = styled.div`
  position: relative;
  overflow: hidden;

  border-radius: ${radius.m};

  box-shadow: 0px 4px 4px 0px rgba(0, 0, 0, 0.10);
`

function Swapper() {
  const { open, setOpen } = useSearchContext()
  const [boxPos, setBoxPos] = React.useState(0)
  return <>
    {!open && <PlaceholderContainer onClick={e => {
      setOpen(true)
      setBoxPos(e.currentTarget.getBoundingClientRect().y)
    }}>
      <Backdrop />
      <SearchBox />
    </PlaceholderContainer>}
    {open && <SearchDialog offsetY={boxPos} onClose={() => setOpen(false)} />}
  </>
}

export default function SearchPlaceholder() {
  const [showSearchBox] = useNewTabPref('showSearchBox')
  if (!showSearchBox) return null
  return <SearchContext>
    <Swapper />
  </SearchContext>
}
