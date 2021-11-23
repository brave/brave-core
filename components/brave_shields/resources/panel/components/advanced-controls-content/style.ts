import styled from 'styled-components'
import globalIconUrl from '../../../../../web-components/icons/globe.svg'
import listsIconUrl from '../../../../../web-components/icons/lists.svg'
import { CaratStrongDownIcon } from 'brave-ui/components/icons'

export const SettingsBox = styled.div`
  padding: 17px 22px;

  a {
    font-size: 13px;
    font-weight: 600;
    text-decoration: none;
    display: flex;
    align-items: center;
    margin-bottom: 20px;

    &:last-child {
      margin-bottom: 0;
    }
  }
`

export const SettingsTitle = styled.div`
  color: ${(p) => p.theme.color.text02};
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 12px;
  letter-spacing: 0.01em;
  margin-bottom: 14px;
`

export const SettingsDesc = styled(SettingsTitle)`
  color: ${(p) => p.theme.color.text03};
`

export const GlobeIcon = styled.i`
  display: block;
  width: 17px;
  height: 17px;
  background-image: url(${globalIconUrl});
  background-repeat: no-repeat;
  background-size: cover;
  background-position: top;
  margin-right: 5px;
`

export const ListIcon = styled(GlobeIcon)`
  height: 12px;
  background-image: url(${listsIconUrl});
`

export const ControlGroup = styled.div`
  display: flex;
  align-items: center;
  margin-bottom: 22px;

  select,
  label {
    width: 100%;
  }

  label {
    display: flex;
    align-items: center;
    justify-content: space-between;
    font-family: ${(p) => p.theme.fontFamily.heading};
    font-size: 12px;
    color: ${(p) => p.theme.color.text01};

    span {
      margin-right: 5px;
    }
  }
`

export const ControlCount = styled.button`
  background-color: transparent;
  border: 0;
  padding: 0;
  display: inline-flex;
  align-items: center;
  font-family: ${(p) => p.theme.fontFamily.heading};
  font-size: 13px;
  font-weight: 600;
  color: ${(p) => p.theme.color.interactive05};
  width: 48px;

  &:disabled,
  [disabled] {
    color: ${(p) => p.theme.color.disabled};
  }

`

export const CaratDown = styled(CaratStrongDownIcon)`
  width: 18px;
  height: auto;
  margin-right: 5px;
`
