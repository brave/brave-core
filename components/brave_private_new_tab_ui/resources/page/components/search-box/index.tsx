// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import styled from 'styled-components'
import { getLocale } from '$web-common/locale'

const Box = styled.div`
  display: flex;
  justify-content: center;
  flex-direction: column;
  align-items: center;
  gap: 40px;
  width: 100%;
`

const Form = styled.form`
  --bg-color: rgba(255, 255, 255, 0.22);
  --box-shadow: 0px 2px 70px rgba(0, 0, 0, 0.3);

  display: grid;
  grid-template-columns: 1fr 50px;
  align-items: center;
  width: 100%;
  height: 52px;
  font-family: ${p => p.theme.fontFamily.heading};
  color: white;
  font-size: 14px;
  font-weight: 400;
  background: var(--bg-color);
  border-radius: 8px;
  transition: box-shadow 0.3s ease-in-out;
  overflow: hidden;

  &:focus-within,
  &:hover {
    box-shadow: var(--box-shadow);
  }

  input[type="text"] {
    width: 100%;
    height: 36px;
    border: 0;
    background-color: transparent;
    padding: 5px 16px;

    &:focus {
      outline: 0;
    }

    &::placeholder {
      color: rgba(255,255,255,0.7);
    }
  }
`

const IconButton = styled.button`
  background: transparent;
  padding: 0;
  margin: 0;
  border: 0;
  width: 100%;
  height: 100%;
  cursor: pointer;

  &:hover {
    background: linear-gradient(304.74deg, #6F4CD2 15.81%, #BF14A2 63.17%, #F73A1C 100%);

    path {
      fill: white;
    }
  }
`

function BraveSearchLogo () {
  return (
    <svg
      width={244}
      height={80}
      fill="none"
      xmlns="http://www.w3.org/2000/svg"
    >
    <path
      fillRule="evenodd"
      clipRule="evenodd"
      d="M81.512 40.373V14.791h2.453a3.614 3.614 0 0 1 3.619 3.61v11.375c3.931-3.5 7.275-4.9 11.73-4.99 8.547-.175 15.964 6.158 16.669 14.657.842 10.154-6.559 17.797-16.202 17.797-10.586 0-18.269-6.549-18.269-16.867Zm5.971 2.01c.599 5.285 5.659 9.65 10.994 9.735 6.951.106 11.688-4.413 11.688-11.1 0-6.686-4.631-11.1-11.386-11.1-7.221-.016-12.117 5.217-11.296 12.464Zm38.63-16.958v4.557c3.82-4.229 6.083-5.365 10.554-5.365 1.301.001 2.6.105 3.884.312v6.052a15.74 15.74 0 0 0-4.207-.56c-3.237 0-5.955 1.057-7.947 3.055-1.749 1.75-2.268 3.118-2.268 5.867v16.85h-6.294V25.426h6.278ZM142.045 35.6c.323-2.997.906-4.62 2.199-6.179 2.591-3.171 7.253-4.804 13.855-4.804 5.245 0 9.066.814 11.72 2.558 2.204 1.432 3.11 3.557 3.11 7.3v13.663c0 2.997.779 4.054 2.914 3.996.477-.017.953-.059 1.425-.127v4.118a9.362 9.362 0 0 1-3.3.497c-3.948 0-5.76-1.184-6.989-4.62-3.884 3.557-7.9 5.053-13.659 5.053-7.704 0-12.43-3.494-12.43-9.176a8.076 8.076 0 0 1 4.853-7.484c2.332-1 3.757-1.311 10.422-1.998 5.696-.56 6.671-.688 8.287-1.19 1.748-.56 2.527-1.495 2.527-3.055 0-3.309-2.782-4.805-8.61-4.805-5.51 0-8.35 1.09-9.489 3.87a3.756 3.756 0 0 1-3.47 2.368l-3.365.015Zm24.902 5.244a61.784 61.784 0 0 1-12.239 2.246c-5.632.56-7.381 1.623-7.381 4.43 0 3.118 2.591 4.757 7.704 4.757 4.207 0 7.381-1.057 9.648-3.119 1.812-1.686 2.268-2.87 2.268-5.814v-2.5Zm25.517 15.344h-1.621a3.71 3.71 0 0 1-3.359-2.083l-13.58-28.68h3.772a5.549 5.549 0 0 1 5.103 3.261l9.06 20.615 9.362-20.694a5.549 5.549 0 0 1 5.076-3.203h3.428l-13.898 28.733a3.71 3.71 0 0 1-3.343 2.051Zm21.496-13.6c.646 5.93 5.298 9.61 12.233 9.61 3.937 0 6.888-1.168 8.716-3.5a5.62 5.62 0 0 1 4.398-2.114h3.65c-3.179 6.988-8.869 10.424-17.352 10.424-10.596 0-17.739-6.428-17.739-16.037 0-9.61 7.317-16.349 17.872-16.349 7.635 0 13.775 3.43 16.25 9.176 1.102 2.643 1.552 4.99 1.552 8.796l-29.58-.006Zm23.174-4.757c-1.425-5.428-5.632-8.489-11.46-8.489-5.828 0-10.035 3.06-11.524 8.49h22.984Z"
      fill="#fff"
    />
    <path
      fillRule="evenodd"
      clipRule="evenodd"
      d="m62.038 17.836 1.746-4.28s-2.222-2.378-4.92-5.073c-2.698-2.695-8.41-1.11-8.41-1.11L43.946 0H21.094l-6.506 7.373s-5.713-1.585-8.411 1.11a230.117 230.117 0 0 0-4.92 5.073l1.746 4.28-2.222 6.34s6.534 24.722 7.3 27.741c1.508 5.945 2.54 8.243 6.824 11.255 4.285 3.012 12.061 8.243 13.33 9.036 1.27.792 2.857 2.142 4.285 2.142 1.429 0 3.016-1.35 4.285-2.142 1.27-.793 9.046-6.024 13.33-9.036 4.286-3.012 5.317-5.31 6.825-11.255.765-3.019 7.3-27.74 7.3-27.74l-2.222-6.341Z"
      fill="url(#brave_search_a)"
    />
    <path
      fillRule="evenodd"
      clipRule="evenodd"
      d="M40.614 13.397c.952 0 8.014-1.348 8.014-1.348s8.37 10.106 8.37 12.266c0 1.786-.72 2.485-1.568 3.308-.178.172-.36.35-.544.544l-6.275 6.657c-.062.066-.132.135-.205.209-.626.629-1.549 1.555-.898 3.094l.134.313c.713 1.667 1.594 3.725.473 5.81-1.193 2.218-3.236 3.698-4.545 3.453-1.31-.244-4.384-1.85-5.515-2.583-1.13-.733-4.714-3.685-4.714-4.815 0-.942 2.577-2.509 3.83-3.27.248-.152.445-.271.558-.348.13-.087.346-.221.612-.386 1.143-.71 3.207-1.99 3.258-2.558.064-.7.04-.906-.881-2.635-.196-.368-.425-.762-.662-1.17-.877-1.507-1.86-3.195-1.642-4.404.245-1.366 2.386-2.148 4.2-2.811.227-.083.449-.164.66-.244.53-.2 1.195-.449 1.89-.709 1.811-.677 3.823-1.43 4.155-1.583.46-.212.341-.413-1.05-.545a40.21 40.21 0 0 1-.676-.07c-1.722-.182-4.899-.52-6.443-.09-.303.085-.644.177-1.001.274-1.735.47-3.861 1.044-4.066 1.376-.035.058-.07.108-.103.155-.196.277-.323.458-.107 1.637.064.351.197 1.042.36 1.896.48 2.5 1.227 6.4 1.321 7.276l.042.355c.12.977.2 1.628-.937 1.888l-.297.068c-1.282.294-3.162.724-3.842.724-.68 0-2.56-.43-3.843-.725l-.295-.067c-1.137-.26-1.057-.91-.937-1.888.014-.114.028-.232.041-.355.095-.877.844-4.787 1.323-7.287.163-.849.295-1.535.359-1.885.216-1.179.089-1.36-.107-1.637a3.187 3.187 0 0 1-.104-.155c-.204-.332-2.33-.907-4.065-1.376-.357-.097-.698-.189-1.002-.273-1.544-.43-4.72-.093-6.442.09-.263.027-.492.052-.676.07-1.391.13-1.51.332-1.05.544.331.153 2.342.905 4.153 1.583l1.89.709c.213.08.435.161.662.244 1.813.663 3.955 1.446 4.2 2.81.218 1.21-.764 2.897-1.642 4.405-.237.407-.466.802-.662 1.17-.921 1.73-.946 1.935-.882 2.635.052.568 2.115 1.848 3.258 2.557.266.166.483.3.612.387.114.077.31.196.56.347 1.251.762 3.83 2.33 3.83 3.271 0 1.13-3.585 4.082-4.715 4.815-1.13.734-4.206 2.339-5.515 2.584-1.31.244-3.352-1.236-4.545-3.454-1.121-2.085-.24-4.143.473-5.81l.133-.313c.651-1.539-.271-2.466-.898-3.095a9.575 9.575 0 0 1-.204-.208l-6.275-6.656a17.27 17.27 0 0 0-.544-.545c-.848-.823-1.567-1.522-1.567-3.308 0-2.16 8.369-12.266 8.369-12.266s7.062 1.348 8.014 1.348c.76 0 2.227-.505 3.756-1.03.387-.134.779-.269 1.163-.397 1.905-.634 3.174-.639 3.174-.639s1.27.005 3.174.64c.385.127.776.262 1.164.395 1.529.526 2.996 1.03 3.756 1.03ZM39.402 49.14c1.493.769 2.552 1.314 2.952 1.564.518.324.202.935-.27 1.268-.471.334-6.813 5.239-7.429 5.78l-.249.224c-.593.534-1.35 1.215-1.886 1.215-.536 0-1.293-.682-1.886-1.216l-.248-.222c-.616-.542-6.958-5.447-7.43-5.78-.471-.334-.787-.945-.27-1.269.401-.25 1.462-.796 2.956-1.565l1.42-.732c2.236-1.156 5.024-2.139 5.458-2.139.435 0 3.222.983 5.459 2.139.508.262.984.508 1.423.733Z"
      fill="#fff"
    />
    <path
      fillRule="evenodd"
      clipRule="evenodd"
      d="M50.453 7.373 43.947 0H21.094l-6.506 7.373s-5.713-1.585-8.411 1.11c0 0 7.617-.687 10.236 3.567 0 0 7.062 1.347 8.014 1.347.952 0 3.015-.792 4.92-1.427 1.904-.634 3.174-.638 3.174-.638s1.27.004 3.174.638c1.904.635 3.967 1.427 4.92 1.427.951 0 8.013-1.347 8.013-1.347 2.619-4.254 10.236-3.567 10.236-3.567-2.697-2.695-8.41-1.11-8.41-1.11Z"
      fill="url(#brave_search_b)"
    />
    <defs>
      <linearGradient
        id="brave_search_a"
        x1={0.781}
        y1={74.875}
        x2={64.26}
        y2={74.875}
        gradientUnits="userSpaceOnUse"
      >
        <stop stopColor="#F50" />
        <stop offset={0.41} stopColor="#F50" />
        <stop offset={0.582} stopColor="#FF2000" />
        <stop offset={1} stopColor="#FF2000" />
      </linearGradient>
      <linearGradient
        id="brave_search_b"
        x1={7.309}
        y1={13.348}
        x2={58.864}
        y2={13.348}
        gradientUnits="userSpaceOnUse"
      >
        <stop stopColor="#FF452A" />
        <stop offset={1} stopColor="#FF2000" />
      </linearGradient>
    </defs>
  </svg>
  )
}

interface Props {
  onSubmit?: (value: string, openNewTab: boolean) => unknown
}

function Search (props: Props) {
  const [value, setValue] = React.useState('')
  const inputRef = React.useRef<HTMLInputElement>(null)

  const onInputChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    setValue(e.currentTarget.value)
  }

  const handleFormBoxClick = () => {
    inputRef.current && inputRef.current.focus()
  }

  const handleSubmit = (e: React.ChangeEvent<HTMLFormElement>) => {
    e.preventDefault()
    props.onSubmit?.(value, false)
  }

  const handleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
    if (value === '') return

    if ((e.metaKey || e.ctrlKey) && (e.key === 'Enter')) {
      props.onSubmit?.(value, true)
    }
  }

  return (
    <Box>
      <BraveSearchLogo />
      <Form onSubmit={handleSubmit} onClick={handleFormBoxClick} role="search" aria-label="Brave">
        <input ref={inputRef} onChange={onInputChange} onKeyDown={handleKeyDown} type="text" placeholder={getLocale('searchPlaceholderLabel')} value={value} autoCapitalize="off" autoComplete="off" autoCorrect="off" spellCheck="false" aria-label="Search" title="Search" aria-autocomplete="none" aria-haspopup="false" maxLength={2048} autoFocus />
        <IconButton data-test-id="submit_button" aria-label="Submit">
          <svg width="20" height="20" fill="none" xmlns="http://www.w3.org/2000/svg"><path fillRule="evenodd" clipRule="evenodd" d="M8 16a8 8 0 1 1 5.965-2.67l5.775 5.28a.8.8 0 1 1-1.08 1.18l-5.88-5.375A7.965 7.965 0 0 1 8 16Zm4.374-3.328a.802.802 0 0 0-.201.18 6.4 6.4 0 1 1 .202-.181Z" fill="url(#search_icon_gr)"/><defs><linearGradient id="search_icon_gr" x1="20" y1="20" x2="-2.294" y2="3.834" gradientUnits="userSpaceOnUse"><stop stopColor="#BF14A2"/><stop offset="1" stopColor="#F73A1C"/></linearGradient></defs></svg>
        </IconButton>
      </Form>
    </Box>
  )
}

export default Search
