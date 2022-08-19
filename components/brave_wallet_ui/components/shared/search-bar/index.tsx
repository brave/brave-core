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
  autoFocus?: boolean
  value?: string
  disabled?: boolean
}

export default class SearchBar extends React.PureComponent<Props> {
  render () {
    const { autoFocus, placeholder, action, value, disabled } = this.props
    return (
      <StyledWrapper>
        <SearchIcon />
        <SearchInput
          autoFocus={autoFocus}
          value={value}
          placeholder={placeholder}
          onChange={action}
          disabled={disabled}
        />
      </StyledWrapper>
    )
  }
}
