import Button from '@brave/leo/react/button';
import ButtonMenu from '@brave/leo/react/buttonMenu';
import Icon from '@brave/leo/react/icon';
import { color, icon, spacing } from "@brave/leo/tokens/css/variables";
import * as React from "react";
import styled from "styled-components";
import Flex from '../../../common/Flex';
import { getLocale } from "../../../common/locale";
import { useSearchContext } from "./SearchContext";
import { MediumSearchEngineIcon } from "./SearchEngineIcon";

const Option = styled.div`
    display: flex;
    color: ${color.black};
    gap: ${spacing.m};
`

const OpenIcon = styled(Icon)`
  --leo-icon-color: rgba(255, 255, 255, 0.5);
  --leo-icon-size: ${icon.xs};
`

const CustomizeButton = styled(Button)`
  border-top: 1px solid ${color.divider.subtle};
  color: ${color.text.secondary};
`

const Vr = styled.div`
  width: 1px;
  background: rgba(255,255,255,0.1);
  height: 24px;
`

const OpenButton = styled(Button)`
  margin: -6px 0 -6px ${spacing.m};
`

export default function EnginePicker() {
  const { filteredSearchEngines, searchEngine, setSearchEngine, setOpen: setBoxOpen } = useSearchContext()
  const [open, setOpen] = React.useState(false)

  return <ButtonMenu data-theme="light" positionStrategy='fixed' isOpen={open} onClose={() => setOpen(false)}>
    <OpenButton fab kind="plain-faint" slot="anchor-content" onClick={() => setOpen(true)}>
      <Flex gap={spacing.s} align='center'>
        <MediumSearchEngineIcon engine={searchEngine} />
        <OpenIcon name={open ? "close" : "carat-down"} />
        <Vr />
      </Flex>
    </OpenButton>
    {filteredSearchEngines.map(s => <leo-menu-item onClick={() => setSearchEngine(s)} key={s.keyword}>
      <Option>
        <MediumSearchEngineIcon engine={s} />{s.name}
      </Option>
    </leo-menu-item>)}
    <CustomizeButton kind="plain-faint" size="small" onClick={() => {
      history.pushState(undefined, '', '?openSettings=Search')

      // For now, close the search box - the Settings dialog doesn't use a
      // dialog, so it gets rendered underneath.
      setBoxOpen(false)
    }}>
      {getLocale('searchCustomizeList')}
    </CustomizeButton>
  </ButtonMenu>
}
