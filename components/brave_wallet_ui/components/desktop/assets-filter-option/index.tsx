import * as React from 'react'

import { Option } from './style'

export interface Props {
  label: string
  value: string
  selected: boolean
  onSelect: (value: string) => void
}

const AssetsFilterOption = (props: Props) => {
  const { selected, label, value, onSelect } = props

  const onClick = () => {
    onSelect(value)
  }

  return (
    <Option
      selected={selected}
      onClick={onClick}
    >
    {label}
    </Option>
  )
}

export default AssetsFilterOption
