import styled from 'styled-components'

export const Box = styled.div`
  border: 1px solid ${p => p.theme.color.divider01};
  border-radius: 2000px;
  padding: 3px;
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  grid-auto-flow: column;
  align-items: center;
  min-height: 48px;

  button {
    font-weight: 400;
    font-size: 14px;
    line-height: 0;
    background-color: transparent;
    padding: 4px 8px;
    border: 0;
    border-radius: 100px;
    width: 100%;
    height: 100%;
    cursor: pointer;
  }

  .sm {
    font-size: 11px;
    line-height: 1.1;
  }

  .is-active {
    background: ${p => p.theme.color.divider01};
  }
`
