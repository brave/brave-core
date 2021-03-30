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
}

export default class SearchBar extends React.PureComponent<Props> {
  render () {
    const { placeholder, action } = this.props
    return (
      <StyledWrapper>
        <SearchIcon />
        <SearchInput
          placeholder={placeholder}
          onChange={action}
        />
      </StyledWrapper>
    )
  }
}
