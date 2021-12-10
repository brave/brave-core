import * as React from 'react'

// Styled Components
import {
  Header,
  HeaderText,
  Button,
  BackIcon,
  HeaderSpacing,
  PlusIcon
} from './style'

export interface Props {
  title: string
  hasAddButton?: boolean
  onClickAdd?: () => void
  onBack: () => void
}

function SelectHeader (props: Props) {
  const { onBack, title, hasAddButton, onClickAdd } = props
  return (
    <Header>
      <Button onClick={onBack}><BackIcon /></Button>
      <HeaderText>{title}</HeaderText>
      {hasAddButton ? (
        <Button onClick={onClickAdd}>
          <PlusIcon />
        </Button>
      ) : (
        <HeaderSpacing />
      )}
    </Header>
  )
}

export default SelectHeader
