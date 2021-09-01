import styled from 'styled-components'

export const Box = styled.div`
  --box-bg: ${(p) => p.theme.color.panelBackground};
  width: 100%;
  height: 100%;
  background: var(--box-bg);
  overflow: hidden;
`

export const PanelHeader = styled.section`
  display: flex;
  align-items: center;
  width: 100%;
  padding: 18px 18px 0 18px;
  margin-bottom: 18px;
  box-sizing: border-box;
`

export const PanelContent = styled.section`
  padding: 0 0 25px 0;
`

export const RegionList = styled.div`
  padding: 0 18px;
  max-height: 320px;
  overflow-y: scroll;

  input {
    /* on input click the browser focuses, which causes a jump
    /* https://stackoverflow.com/a/49452792
    */
    top: auto;
    left: -9999px;
  }

  &::-webkit-scrollbar {
    -webkit-appearance: none;
  }

  &::-webkit-scrollbar:vertical {
    width: 8px;
  }

  &::-webkit-scrollbar-thumb {
    border-radius: 8px;
    border: 2px solid var(--box-bg);
    background-color: ${(p) => p.theme.color.divider01};
  }
`

export const RegionLabel = styled.span`
  color: ${(p) => p.theme.color.text01};
  font-family: Poppins;
  font-size: 14px;
`

export const BackButton = styled.button`
  background-color: transparent;
  border: 0;
  width: 18px;
  height: 18px;
  padding: 0;

  svg > path {
    fill: ${(p) => p.theme.color.interactive05}
  }
`
