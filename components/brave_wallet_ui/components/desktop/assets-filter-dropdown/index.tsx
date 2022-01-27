import { AssetFilter } from '../../../constants/types'
import * as React from 'react'

import { StyledWrapper, Button, CaratDown, Dropdown } from './style'
import AssetsFilterOption from '../assets-filter-option'

export interface Props {
  options: AssetFilter[]
  value: string
  closeOnSelect?: boolean
  onSelectFilter: (value: string) => void
}

const AssetsFilterDropdown = (props: Props) => {
  const { options, value, closeOnSelect, onSelectFilter } = props
  const [isOpen, setIsOpen] = React.useState(false)

  const buttonLabel = React.useMemo(() => {
    const selected = options.find(option => option.value === value)

    return selected !== undefined ? selected.label : ''
  }, [value])

  const onClick = () => {
    setIsOpen(!isOpen)
  }

  const onOptionSelect = (value: string) => {
    if (closeOnSelect) {
      setIsOpen(false)
    }

    onSelectFilter(value)
  }

  return (
    <StyledWrapper>
      <Button onClick={onClick}>
        {buttonLabel}
        <CaratDown />
      </Button>
      {isOpen &&
        <Dropdown>
          {options.map(option =>
            <AssetsFilterOption
              key={option.value}
              label={option.label}
              value={option.value}
              selected={value === option.value}
              onSelect={onOptionSelect}
            />
          )}
        </Dropdown>
      }
    </StyledWrapper>
  )
}

AssetsFilterDropdown.defaultProps = {
  closeOnSelect: true
}

export default AssetsFilterDropdown
