import styled from 'styled-components'

interface StyleProps {
  floating?: boolean
}

export const StyledSelect = styled<StyleProps, 'select'>('select')`
  width: 100%;
  background: #fff;
  height: 34px;
  font-size: 14px;
  border: ${p => p.floating ? 'none' : '1px solid #DFDFE8'};
  text-align-last: ${p => p.floating ? 'right' : 'left'};

  &:focus {
    outline: 0;
  }
`
