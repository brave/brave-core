import * as React from 'react'
import { useState } from "react";
import styled from "styled-components";
import { FeedSettings, defaultFeedSettings, loadSetting } from "./buildFeed";
import Button from '@brave/leo/react/button'

const Column = styled.div`
  display: flex;
  flex-direction: column;
  gap: 8px;
`

function Tweaker(props: { name: string, defaultValue: number }) {
  const [value, setValue] = useState(loadSetting(props.name as keyof FeedSettings));

  const set = (v: string | number) => {
    setValue(v as any)
    const asNum = parseFloat('' + v)
    localStorage.setItem(props.name, '' + (isNaN(asNum) ? props.defaultValue : asNum))
  }
  return <label>
    {props.name}:&nbsp;
    <input type="text" value={value} onChange={e => set(e.target.value)} />
    {value != props.defaultValue && <Button size='tiny' kind='plain' onClick={() => set(props.defaultValue)}>Reset</Button>}
  </label>
}

export default function Config() {
  const settings = defaultFeedSettings
  return <Column>
    {Object.entries(settings).map(([k, v]) => <Tweaker key={k} name={k} defaultValue={v} />)}
    <Button onClick={() => window.location.reload()}>Rebuild</Button>
  </Column>
}
