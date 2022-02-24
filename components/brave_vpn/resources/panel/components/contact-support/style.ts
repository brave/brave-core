import styled from 'styled-components'

export const Box = styled.div`
  width: 100%;
  height: 100%;
  background: ${(p) => p.theme.color.panelBackground};
  overflow: hidden;
`

export const List = styled.ul`
  list-style-type: none;
  padding: 0;
  margin: 0;
  text-align: center;

  li {
    margin-bottom: 28px;

    &:last-child {
      margin-bottom: 0;
    }
  }

  a {
    font-family: ${(p) => p.theme.fontFamily.heading};
    font-size: 13px;
    font-weight: 600;
    color: ${(p) => p.theme.color.interactive05};
    text-decoration: none;
  }
`

export const Card = styled.li`
  --divider-color: ${(p) => p.theme.color.divider01};
  border: 1px solid var(--divider-color);
  border-radius: 8px;
`

export const PanelContent = styled.section`
  padding: 25px 24px 25px 24px;
  z-index: 2;
`

export const PanelHeader = styled.section`
  display: flex;
  align-items: center;
  width: 100%;
  margin-bottom: 18px;
  box-sizing: border-box;
`

export const BackButton = styled.button`
  background-color: transparent;
  border: 0;
  padding: 0;
  display: flex;
  align-items: center;

  span {
    font-family: ${(p) => p.theme.fontFamily.heading};
    font-size: 13px;
    font-weight: 600;
    color: ${(p) => p.theme.color.interactive05};
  }

  i {
    width: 18px;
    height: 18px;
    margin-right: 5px;
  }

  svg > path {
    fill: ${(p) => p.theme.color.interactive05};
  }
`
