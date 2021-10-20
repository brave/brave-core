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
  onAddAccount?: () => void
  onBack: () => void
}

function SelectHeader (props: Props) {
  const { onBack, title, hasAddButton, onAddAccount } = props
  return (
    <Header>
      <Button onClick={onBack}><BackIcon /></Button>
      <HeaderText>{title}</HeaderText>
      {hasAddButton ? (
        <Button onClick={onAddAccount}>
          <PlusIcon />
        </Button>
      ) : (
        <HeaderSpacing />
      )}
    </Header>
  )
}

export default SelectHeader
