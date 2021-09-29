import * as React from 'react'

// Styled Components
import {
  StyledWrapper,
  SearchInput,
  SearchIcon
} from './style'

export interface Props {
  placeholder: string
  action?: (event: any) => void | undefined
  value?: string
}

export default class SearchBar extends React.PureComponent<Props> {
  render () {
    const { placeholder, action, value } = this.props
    return (
      <StyledWrapper>
        <SearchIcon />
        <SearchInput
          value={value}
          placeholder={placeholder}
          onChange={action}
        />
      </StyledWrapper>
    )
  }
}
