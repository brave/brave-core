import * as React from 'react'
import { useState } from "react";
import styled from "styled-components";
import { defaultFeedSettings } from "./buildFeed";
import Button from '@brave/leo/react/button'

const Column = styled.div`
  display: flex;
  flex-direction: column;
  gap: 8px;
`

function Tweaker(props: { name: string, defaultValue: number }) {
  const [value, setValue] = useState(parseFloat(localStorage.getItem(props.name) || '') || props.defaultValue);

  return <label>
    {props.name}:&nbsp;
    <input type="text" value={value} onChange={e => {
      setValue(e.target.value as any)
      const asNum = parseFloat(e.target.value)
      localStorage.setItem(props.name, '' + (isNaN(asNum) ? props.defaultValue : asNum))
    }} />
  </label>
}

export default function Config() {
  const settings = defaultFeedSettings
  return <Column>
    {Object.entries(settings).map(([k, v]) => <Tweaker key={k} name={k} defaultValue={v} />)}
    <Button onClick={() => window.location.reload()}>Rebuild</Button>
  </Column>
}
