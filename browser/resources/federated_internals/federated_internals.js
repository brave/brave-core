import {$} from 'chrome://resources/js/util.m.js';
import { FederatedInternalsBrowserProxy } from './federated_internals_browser_proxy.js';

const dataStoresLogs = {};
let selectedDataStore = 'notification-ad-timing';

function getProxy() {
  return FederatedInternalsBrowserProxy.getInstance();
}

function initialize() {
  getProxy().updateDataStoresInfo();
  
  getProxy().getCallbackRouter().onUpdateDataStoresInfo.addListener(
    (logs) => {
      dataStoresLogs['notification-ad-timing'] = logs;
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

    if (selectedDataStore == 'notification-ad-timing') {
      $('service-message').textContent = ''
      const thead = $('data-store-headers');
      while (thead.firstChild) {
        thead.removeChild(thead.firstChild);
      }

      let title = "trainingInstanceId"
      const th = document.createElement('th');
      th.textContent = title;
      th.className = 'data-store-feature-'+ title;
      thead.appendChild(th);
      Object.keys(logs[0].covariates[0]).forEach(function(title) {
          const th = document.createElement('th');
          th.textContent = title;
          th.className = 'data-store-feature-'+ title;
          thead.appendChild(th);
      });

      console.log(logs);

      for (const [training_instance_id, training_instance] of Object.entries(logs)) {
        training_instance.covariates.forEach(function(covariate) {
          const tr = document.createElement('tr');
          appendTD(tr, training_instance_id, 'data-store-feature-trainingInstanceId');
          appendTD(tr, covariate.type, 'data-store-feature-type');
          appendTD(tr, covariate.dataType, 'data-store-feature-dataType');
          appendTD(tr, covariate.value, 'data-store-feature-value');
      
          const tabpanel = $('data-store-tab');
          const tbody = tabpanel.getElementsByTagName('tbody')[0];
          tbody.appendChild(tr);
        });
      }
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
  var data = 'training_instance_id,type,data_type,value\n'
  for (const [training_instance_id, training_instance] of Object.entries(dataStoresLogs[selectedDataStore])) {
    training_instance.covariates.forEach(function(covariate) {
      data += training_instance_id.toString() + ',' + covariate.type.toString() + ',' + 
            covariate.dataType.toString() + ',' + covariate.value.toString() + '\n';
    });
  }
  const blob = new Blob([data], {'type': 'text/csv'});
  const url = URL.createObjectURL(blob);
  const filename = selectedDataStore + '_dump.csv';

  const a = document.createElement('a');
  a.setAttribute('href', url);
  a.setAttribute('download', filename);

  const event = document.createEvent('MouseEvent');
  event.initMouseEvent(
      'click', true, true, window, 0, 0, 0, 0, 0, false, false, false, false, 0, null);
  a.dispatchEvent(event);
}

document.addEventListener('DOMContentLoaded', initialize);
