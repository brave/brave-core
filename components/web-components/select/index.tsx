import * as React from 'react'

import * as S from './style'

type ValueType = string | number

interface BaseSelectProps {
  value?: ValueType
  onChange?: (value: ValueType) => unknown
  ariaLabel: string
}

type SelectProps = React.PropsWithChildren<BaseSelectProps>

function Select (props: SelectProps) {
  const handleChange = (e: React.FormEvent<HTMLSelectElement>) => {
    const target = e.target as HTMLSelectElement
    props.onChange?.(target.value)
  }

  return (
    <S.SelectBox>
      <S.Select
        value={props.value}
        onChange={handleChange}
        aria-label={props.ariaLabel}
      >
        {props.children}
      </S.Select>
      <svg width="13" height="7" fill="currentColor" xmlns="http://www.w3.org/2000/svg"><path d="M6.718 4.781 11.624.693a.667.667 0 1 1 .854 1.024L7.145 6.161a.667.667 0 0 1-.854 0L.958 1.717A.667.667 0 0 1 1.81.693L6.718 4.78Z"/></svg>
    </S.SelectBox>
  )
}

export default Select
