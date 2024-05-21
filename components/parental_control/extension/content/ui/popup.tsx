// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

const Popup: React.FC = () => {
  return (
    <div>
      <style>
        {`
      body{
        width: 260px;
        height: 200px;
        padding-bottom: 10%;
      }
      
      #container{
        display: flex;
        align-items: center;
        justify-content: center;
      }
      
      .status{
        margin-bottom: 10px;
      }
      
      form{
        display: flex;
          flex-direction: column;
      }
      
      input{
        padding: 8px;
        border-radius: 5px;
        border: 2px solid #ece2d4;
        margin-top: 10px;
        display: flex;
      }
      
      input:focus{
        outline: none;
        border-color: rgb(255, 190, 51);
        box-shadow: 0 0 3px rgb(255, 190, 51);
        -moz-box-shadow: 0 0 3px rgb(255, 190, 51);
        -webkit-box-shadow: 0 0 3px rgb(255, 190, 51);
      }
      
      input[type="checkbox"]:checked {
        background-color: #ffd000;
      }
      
      .checkbox-container input[type="checkbox"],
              .checkbox-container label {
                  display: inline;
                  vertical-align: middle;
                  font-weight: bold; 
                  font-size: large;
              }
              .checkbox-container{
                  margin-bottom: 10px;
              }
      
      button{
        margin-top: 25px;
        margin-bottom: 20px;
        padding: 0.6rem;
        padding-left: .8rem;
        padding-right: .8rem;
        background-color: rgb(252, 181, 88);
        border-radius: 10px;
        cursor: pointer;
        border-style: none;
        width: 100%;
      }
      
      `}
      </style>
      <div id='container' style={{ display: 'flex', flexDirection: 'column' }}>
        <div id='popupContainers'>
          <div id='signUpContainer'>
            <div style={{ display: 'flex', justifyContent: 'center' }}>
              <h1 style={{ fontSize: '18px', marginBottom: '15px' }}>
                Kids mode
              </h1>
            </div>
            <form id='registerForm'>
              <input type='password' id='password' placeholder='Password' />
              <input
                style={{ marginTop: '10px' }}
                type='password'
                id='cpassword'
                placeholder='Confirm password'
              />
              <button type='submit' id='loginBtn'>
                Set password
              </button>
            </form>
          </div>
          <div id='signInContainer'>
            <div style={{ display: 'flex', justifyContent: 'center' }}>
              <h1 style={{ fontSize: '18px', marginBottom: '15px' }}>
                Kids mode
              </h1>
            </div>
            <form id='loginForm'>
              <input
                type='password'
                id='loginPassword'
                placeholder='Password'
              />
              <div style={{ display: 'flex', justifyContent: 'start' }}>
                <span
                  id='resetPasswordLink'
                  style={{
                    color: '#a35e15',
                    cursor: 'pointer',
                    marginLeft: '3px',
                    marginTop: '2px'
                  }}
                >
                  Reset password
                </span>
              </div>
              <div
                style={{
                  display: 'flex',
                  flexDirection: 'column',
                  marginBottom: '10px',
                  marginTop: '15px'
                }}
              >
                <div className='checkbox-container'>
                  <input
                    type='checkbox'
                    id='blockGamesCheckbox'
                    style={{ margin: '5px' }}
                  />
                  <label
                    className='checkbox-label'
                    htmlFor='blockGamesCheckbox'
                  >
                    Block games
                  </label>
                </div>
                <div className='checkbox-container'>
                  <input
                    type='checkbox'
                    id='blockSocialMediaCheckbox'
                    style={{ margin: '5px' }}
                  />
                  <label
                    className='checkbox-label'
                    htmlFor='blockSocialMediaCheckbox'
                  >
                    Block social media
                  </label>
                </div>
                <div className='session-time-container'>
                  <label htmlFor='sessionTime' className='timerLabel'>
                    Set timer
                  </label>
                  <input
                    id='sessionTime'
                    type='number'
                    defaultValue={1}
                    min={1}
                    max={24}
                    name='sessionTime'
                  />
                  <span
                    id='sessionTimeSpan'
                    style={{
                      display: 'flex',
                      alignItems: 'center',
                      marginLeft: '2px',
                      fontWeight: 'bold'
                    }}
                  >
                    Hr
                  </span>
                </div>
              </div>
              <button type='submit' id='loginBtn'>
                Start kids mode
              </button>
            </form>
          </div>
          <div id='kidsContent'>
            <div style={{ display: 'flex', justifyContent: 'center' }}>
              <h1 style={{ fontSize: '18px', marginBottom: '20px' }}>
                Exit kids mode
              </h1>
            </div>
            <form id='logoutForm'>
              <input
                type='password'
                id='logoutPassword'
                placeholder='Password'
              />
              <button
                type='submit'
                style={{ padding: '10px', marginBottom: '20px' }}
                id='logoutBtn'
              >
                Logout
              </button>
            </form>
            <div
              style={{
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center'
              }}
            >
              <div id='timerDisplay' style={{ fontSize: '16px' }}></div>
            </div>
          </div>
        </div>
        <div id='successStatus' style={{ display: 'block' }}></div>
        <div
          id='errorStatus'
          style={{ display: 'block', height: '10px' }}
        ></div>
      </div>
    </div>
  )
}

export default Popup
