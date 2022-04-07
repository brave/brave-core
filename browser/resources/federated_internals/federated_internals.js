import {$} from 'chrome://resources/js/util.m.js';
import { FederatedInternalsBrowserProxy } from './federated_internals_browser_proxy.js';

const dataStoresLogs = {};
let selectedDataStore = 'ad-timing';

function getProxy() {
  return FederatedInternalsBrowserProxy.getInstance();
}

function initialize() {
  getProxy().getAdStoreInfo();
  
  getProxy().getCallbackRouter().onAdStoreInfoAvailable.addListener(
    (logs) => {
      dataStoresLogs['ad-timing'] = logs;
      onDataStoreChanged();
  });

  const button = $('data-store-logs-dump');
  button.addEventListener('click', onLogsDump);

  $('stores').onchange = onDataStoreChanged;
}

function onDataStoreChanged() {
    let store_selection = $('stores')
    selectedDataStore = store_selection.options[store_selection.selectedIndex].value;
    const logs = dataStoresLogs[selectedDataStore];

    if (logs.length == 0) {
      $('service-message').textContent = "No logs for selected Data Store."
      return;
    }

    if (selectedDataStore == 'ad-timing') {
      $('service-message').textContent = ''
      const thead = $('ad-timing-headers');
      while (thead.firstChild) {
        thead.removeChild(thead.firstChild);
      }
      Object.keys(logs[0]).forEach(function(title) {
          const th = document.createElement('th');
          th.textContent = title;
          th.className = 'ad-timing-log-'+ title;
      
          const thead = $('ad-timing-headers');
          thead.appendChild(th);
      });
    
      logs.forEach(function(log) {
        const tr = document.createElement('tr');
        appendTD(tr, log.logId, 'ad-timing-log-id');
        appendTD(tr, formatDate(new Date(log.logTime)), 'ad-timing-log-time');
        appendTD(tr, log.logLocale, 'ad-timing-log-locale');
        appendTD(tr, log.logNumberOfTabs, 'ad-timing-log-number_of_tabs');
        appendBooleanTD(tr, log.logLabel, 'ad-timing-log-label');
    
        const tabpanel = $('ad-timing-tab');
        const tbody = tabpanel.getElementsByTagName('tbody')[0];
        tbody.appendChild(tr);
      });
    }
}

// COSMETICS

function appendTD(parent, content, className) {
  const td = document.createElement('td');
  td.textContent = typeof(content) === 'number' ? content.toString() : content;
  td.className = className;
  parent.appendChild(td);
}

function appendBooleanTD(parent, value, className) {
  const td = document.createElement('td');
  td.textContent = value ? 'True' : 'False';
  td.className = className;
  td.bgColor = value ? '#3cba54' : '#db3236';
  parent.appendChild(td);
}

function padWithZeros(number, width) {
  const numberStr = number.toString();
  const restWidth = width - numberStr.length;
  if (restWidth <= 0) {
    return numberStr;
  }

  return Array(restWidth + 1).join('0') + numberStr;
}

function formatDate(date) {
  const year = date.getFullYear();
  const month = date.getMonth() + 1;
  const day = date.getDate();
  const hour = date.getHours();
  const minute = date.getMinutes();
  const second = date.getSeconds();

  const yearStr = padWithZeros(year, 4);
  const monthStr = padWithZeros(month, 2);
  const dayStr = padWithZeros(day, 2);
  const hourStr = padWithZeros(hour, 2);
  const minuteStr = padWithZeros(minute, 2);
  const secondStr = padWithZeros(second, 2);

  return yearStr + '-' + monthStr + '-' + dayStr + ' ' + hourStr + ':' +
      minuteStr + ':' + secondStr;
}

function onLogsDump() {
  const data = JSON.stringify(dataStoresLogs[selectedDataStore]);
  const blob = new Blob([data], {'type': 'text/json'});
  const url = URL.createObjectURL(blob);
  const filename = selectedDataStore + 'dump.json';

  const a = document.createElement('a');
  a.setAttribute('href', url);
  a.setAttribute('download', filename);

  const event = document.createEvent('MouseEvent');
  event.initMouseEvent(
      'click', true, true, window, 0, 0, 0, 0, 0, false, false, false, false, 0, null);
  a.dispatchEvent(event);
}

document.addEventListener('DOMContentLoaded', initialize);
