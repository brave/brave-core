import * as React from 'react'

// Styled Components
import {
  Header,
  HeaderText,
  BackButton,
  BackIcon,
  HeaderSpacing
} from './style'

export interface Props {
  title: string
  onBack: () => void
}

function SelectHeader (props: Props) {
  const { onBack, title } = props
  return (
    <Header>
      <BackButton onClick={onBack}><BackIcon /></BackButton>
      <HeaderText>{title}</HeaderText>
      <HeaderSpacing />
    </Header>
  )
}

export default SelectHeader
