
/* global $, chrome, alert */

console.log('brave_sync.js loaded')

window.cr.define('sync_ui_exports', function () {
  'use strict'

  function initialize () {
    console.log('brave_sync.js initialize()')
    $('welcome-message').textContent = 'Welcome Here'

    // 'area_header' always shown
    $('btn_new_to_sync').onclick = btnNewToSyncOnClick
    $('btn_have_sync_code').onclick = btnHaveSyncCodeOnClick
    $('btn_setup_sync_have_code').onclick = btnSetupSyncHaveCodeOnClick
    $('btn_setup_sync_new_to_sync').onclick = btnSetupSyncNewToSyncOnClick

    $('btn_reset_sync').onclick = btnResetSyncOnClick
    $('btn_sync_new_device').onclick = btnSyncNewDeviceOnClick

    $('btn_add_mobile_device').onclick = btnAddMobileDeviceOnClick
    $('btn_add_computer').onclick = btnAddComputerOnClick
    $('btn_add_mobile_device_done').onclick = btnAddMobileDeviceDoneOnClick
    $('btn_add_computer_copy_to_clipboard').onclick = btnAddComputerCopyToClipboardOnClick
    $('btn_add_computer_done').onclick = btnAddComputerDoneOnClick

    $('btn_test').onclick = btnTestOnClick

    $('area_new_to_sync').style.display = 'none'
    $('area_have_sync_code').style.display = 'none'
    $('area_buttons_start').style.display = 'none'
    $('area_sync_this_device').style.display = 'none'
    $('area_devices_list').style.display = 'none'
    $('area_sync_data_types').style.display = 'none'
    $('area_clear_data').style.display = 'none'

    $('area_add_another_device').style.display = 'none'
    $('area_add_another_device_type').style.display = 'none'
    $('area_add_mobile_device').style.display = 'none'
    $('area_add_computer').style.display = 'none'

    chrome.send('pageLoaded')
  } // function initialize

  function showSettings (settings, devices) {
    console.log('brave_sync.js showSettings settings=', settings)
    console.log('brave_sync.js showSettings devices=', devices)

    console.log('brave_sync.js showSettings settings.this_deviceName=' + settings.this_deviceName)
    console.log('brave_sync.js showSettings settings.sync_this_device=' + settings.sync_this_device)
    console.log('brave_sync.js showSettings settings.sync_settings=' + settings.sync_settings)
    console.log('brave_sync.js showSettings settings.sync_history=' + settings.sync_history)
    console.log('brave_sync.js showSettings settings.sync_bookmarks=' + settings.sync_bookmarks)

    console.log('brave_sync.js showSettings settings.sync_configured=' + settings.sync_configured)

    if (settings.sync_configured) {
      showSettingsConfigured(settings, devices)
    } else {
      showSettingsNotConfigured(settings, devices)
    }
  } // function showSettings(settings, devices)

  function showSettingsNotConfigured (settings, devices) {
    console.log('brave_sync.js showSettingsForNotConfigured')
    $('area_buttons_start').style.display = 'block'
    $('area_new_to_sync').style.display = 'none'
    $('area_have_sync_code').style.display = 'none'
    $('area_sync_this_device').style.display = 'none'
    $('area_devices_list').style.display = 'none'
    $('area_sync_data_types').style.display = 'none'
    $('area_clear_data').style.display = 'none'
  }

  function showSettingsConfigured (settings, devices) {
    console.log('brave_sync.js showSettingsConfigured')
    $('area_buttons_start').style.display = 'none'
    $('area_new_to_sync').style.display = 'none'
    $('area_have_sync_code').style.display = 'none'

    $('area_sync_this_device').style.display = 'block'
    $('area_devices_list').style.display = 'block'
    $('area_sync_data_types').style.display = 'block'
    $('area_clear_data').style.display = 'block'

    $('checkbox_sync_this_device').checked = settings.sync_this_device
    $('text_this_deviceName').textContent = settings.this_deviceName
    $('checkbox_bookmarks').checked = settings.sync_bookmarks
    $('checkbox_history').checked = settings.sync_history
    $('checkbox_site_settings').checked = settings.sync_settings

    // Apply devices
    var table = $('table_devices')
    // Show existing
    console.log('brave_sync.js showSettingsConfigured table.rows.length=', table.rows.length)
    var sum = ''
    var rows = table.rows
    for (var i = 0; i < rows.length; ++i) {
      var row = rows[i]
      var cells = row.cells
      for (var j = 0; j < cells.length; ++j) {
        var cell = cells[j]
        sum += cell.innerHTML
        sum += '~'
      }
      sum += '~'
    }
    console.log('brave_sync.js showSettingsConfigured existing table', sum)

    // Clean all
    rows = table.rows
    for (; rows.length > 1;) {
      table.deleteRow(rows.length - 1)
    }

    // Add elements according to devices
    for (var ix = 0; ix < devices.length; ++ix) {
      var device = devices[i]
      var rowz = table.insertRow(table.rows.length)
      var cellId = rowz.insertCell(0)
      var deviceId = device.deviceId
      cellId.innerHTML = deviceId
      var cellName = rowz.insertCell(1)
      cellName.innerHTML = device.name
      var cellLastActive = rowz.insertCell(2)
      var laStr = (new Date(device.last_active)).toString()
      cellLastActive.innerHTML = laStr

      var cellDelete = rowz.insertCell(3)

      cellDelete.innerHTML = '<a href="#" id="link_delete_device_' + deviceId + '" >DELETE</a>'
      $('link_delete_device_' + deviceId).onclick = makeDeleteFunction(deviceId)
    }
  }

  function haveSyncWords (syncWords) {
    console.log('brave_sync.js haveSyncWords syncWords=', syncWords)
    $('edit_add_computer_sync_phrase').value = syncWords
  }

  function toHexString (byteArray) {
    return Array.from(byteArray, function (byte) {
      return ('0' + (byte & 0xFF).toString(16)).slice(-2)
    }).join('')
  }

  function haveSeedForQrCode (seed) {
    console.log('brave_sync.js haveSeedForQrCode=', seed)
    // split -> hex -> qr
    var arrInt = seed.split(',').map(Number)
    var s = toHexString(arrInt)
    var qr = qrcode(4, 'L')  // TODO, AB: use `qr-image` instead of `qrcode-generator` ?
    qr.addData(s)
    qr.make()
    var cellSize = 8
    $('area_add_mobile_device_qrcode').innerHTML = qr.createImgTag(cellSize)
  }

  function logMessage (message) {
    console.log('brave_sync.js logMessage =', message)
    $('box_log').innerHTML += (message + '<br />')
  }

  function testClickedResponse (arg1, arg2, arg3, arg4) {
    console.log('brave_sync.js testClickedResponse arg1=', arg1)
    console.log('brave_sync.js testClickedResponse arg2=', arg2)
    console.log('brave_sync.js testClickedResponse arg3=', arg3)
    console.log('brave_sync.js testClickedResponse arg4=', arg4)
  }

  // Return an object with all of the exports.
  return {
    initialize: initialize,
    showSettings: showSettings,
    haveSyncWords: haveSyncWords,
    haveSeedForQrCode: haveSeedForQrCode,
    logMessage: logMessage,
    testClickedResponse: testClickedResponse
  }
})

document.addEventListener('DOMContentLoaded', window.sync_ui_exports.initialize)

function deleteDevice (deviceId) {
  chrome.send('deleteDevice', [deviceId])
}

function makeDeleteFunction (deviceId_) {
  return function () {
    deleteDevice(deviceId_)
  }
}

function btnNewToSyncOnClick () {
  console.log('Handler btnNewToSyncOnClick() called.')
  $('area_buttons_start').style.display = 'none'
  $('area_new_to_sync').style.display = 'block'
}

function btnHaveSyncCodeOnClick () {
  console.log('Handler btnHaveSyncCodeOnClick() called.')
  $('area_buttons_start').style.display = 'none'
  $('area_have_sync_code').style.display = 'block'
}

function btnSetupSyncHaveCodeOnClick () {
  console.log('Handler btnSetupSyncHaveCodeOnClick() called.')
  var syncWords = $('edit_syncWords').value
  var deviceName = $('edit_deviceName_have_code').value
  console.log('syncWords=', syncWords)
  console.log('deviceName=', deviceName)

  chrome.send('setupSyncHaveCode', [syncWords, deviceName])
}

function btnSetupSyncNewToSyncOnClick () {
  console.log('Handler btnSetupSyncNewToSyncOnClick() called.')
  var deviceName = $('edit_deviceName_new_to_sync').value
  console.log('deviceName=', deviceName)
  chrome.send('setupSyncNewToSync', [deviceName])
}

function btnResetSyncOnClick () {
  console.log('Handler btnResetSyncOnClick() called.')
  chrome.send('resetSync')
}

function btnSyncNewDeviceOnClick () {
  $('area_add_another_device').style.display = 'block'
  $('area_add_another_device_type').style.display = 'block'
}

function btnAddMobileDeviceOnClick () {
  $('area_add_another_device_type').style.display = 'none'
  $('area_add_mobile_device').style.display = 'block'
  $('area_add_computer').style.display = 'none'
  chrome.send('needSyncQRcode')
}

function btnAddComputerOnClick () {
  $('area_add_another_device_type').style.display = 'none'
  $('area_add_mobile_device').style.display = 'none'
  $('area_add_computer').style.display = 'block'
  chrome.send('needSyncWords')
}

function btnAddMobileDeviceDoneOnClick () {
  alert('Add mobile device - Done')
}

function btnAddComputerCopyToClipboardOnClick () {
  var copyText = $('edit_add_computer_sync_phrase')
  copyText.select()
  document.execCommand('copy')
  alert('Copied the text: ' + copyText.value)
}

function btnAddComputerDoneOnClick () {
  alert('Add computer - Done')
}

function btnTestOnClick () {
  alert('Test')
  chrome.send('testClicked')
}
