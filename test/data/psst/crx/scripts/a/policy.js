// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const curUrl = window.location.href
console.log("[PSST POLICY SCRIPT] Current URL: " + curUrl);
// Timeout to wait of the URL opening
const WAIT_FOR_PAGE_TIMEOUT = 1000
const WAIT_FOR_PAGE_ATTEMPTS_COUNT = 6

// Use tasks as list of the policy settings tasks to apply
const PSST_TASKS = params.tasks
const PSST_TASKS_LENGTH = params.tasks?.length ?? 0

// Flag which is present only for the first (initial) execution of the policy script
const PSST_INITIAL_EXECUTION_FLAG = params.initial_execution ?? false

const PSST_CHECK_SETTINGS_LOADED = params.psst_settings_status ?? null

const PSST_LOCALSTORAGE_KEY = 'psst'

// State of operations
const psstState = {
  STARTED: "started",
  COMPLETED: "completed"
}

/* Helper functions */
const checkCheckboxes = (resolve, reject, turnOff) => {
  const checkboxes = document.querySelectorAll("input[type='checkbox']")
  if (checkboxes.length === 1) {
    if (turnOff) {
      if (checkboxes[0].checked) {
        // Uncheck it
        checkboxes[0].click()
      }
    }
    resolve(true)
  } else {
    // Throw error
    reject('No checkbox found')
  }
}

const waitForCheckboxToLoadWithTimeout = (turnOff) => {
  return new Promise((resolve, reject) => {
    let intervalId = null
    let attemptCount = 0
    
    const wrappedResolve = (value) => {
      if (intervalId) clearInterval(intervalId)
      resolve(value)
    }
    
    const wrappedReject = (error) => {
      attemptCount++
      if (attemptCount >= WAIT_FOR_PAGE_ATTEMPTS_COUNT) {
        if (intervalId) clearInterval(intervalId)
        reject(`Checkbox not found after ${WAIT_FOR_PAGE_ATTEMPTS_COUNT} attempts`)
      }
    }
    
    intervalId = setInterval(() => {
      checkCheckboxes(wrappedResolve, wrappedReject, turnOff)
    }, WAIT_FOR_PAGE_TIMEOUT)
  })
}

const getAvailableTasks = (psst) => {
  const tasksInList = (psst?.tasks_list?.length ?? 0)
  console.log('[PSST] getAvailableTasks tl:' + (psst?.tasks_list?.length ?? 0) + ' ct:' + ((psst?.current_task ?? null) === null ? 0 : 1))
  return tasksInList + ((psst?.current_task ?? null) === null ? 0 : 1)
}

const getProcessedTasks = (psst) => {
    return (psst?.applied_tasks?.length ?? 0) + (psst?.errors?.length ?? 0)
}

const calculateProgress = (psstObj) => {
  const processed = Number(getProcessedTasks(psstObj)) || 0
  const available = Number(getAvailableTasks(psstObj)) || 0
  const total = processed + available
  
  console.log(`[PSST] calculateProgress processed:${processed} available:${available}`)

  return total === 0 ? 0 : (processed / total) * 100
}

const clearPolicyResults = () => {
  const prefix = "psst_settings_status";
  const storage = window.parent.localStorage;
  
  Object.keys(storage)
    .filter(k => k.startsWith(prefix))
    .forEach(k => storage.removeItem(k));
};

const setResultToWindow = (result) => {
  console.log(`[PSST] setResultToWindow psst_settings_status_${PSST_CHECK_SETTINGS_LOADED}, result:${JSON.stringify(result)}`);
  window.parent.localStorage.setItem(`psst_settings_status_${PSST_CHECK_SETTINGS_LOADED}`, JSON.stringify(result))
}

const getResult = (result, psst, nextUrl) => {
  const result_value = {
    result: result,
    psst: psst,
    next_url: nextUrl
  };
  console.log("[PSST POLICY SCRIPT] Result:", JSON.stringify(result_value));
   return result_value;
}

const start = () => {
  console.log(`[PSST] start #100 tasks:`, PSST_TASKS ?? []);

  // Ensure we have an array and safely get the first task (if any)
  const tasks = Array.isArray(PSST_TASKS) ? [...PSST_TASKS] : [];
  const next_task = tasks.shift() || null;

  const psst = {
    state: psstState.STARTED,
    tasks_list: tasks,
    start_url: window.location.href,
    progress: 0,
    current_task: next_task,
    applied_tasks: []
  };

  return [psst, next_task?.url ?? null];
};

const save = (psst) => {
  // Save the psst object to local storage.
  window.parent.localStorage.setItem(PSST_LOCALSTORAGE_KEY, JSON.stringify(psst))
}

const moveCurrentTask = (psstObj, checkboxResult) => {
  const current_task = psstObj.current_task
  if(!current_task) {
    return
  }
  psstObj.applied_tasks.push(!checkboxResult ? psstObj.current_task : {
    url: current_task.url,
    description: current_task.description,
    error_description: checkboxResult
  })
}

(async() => {
  const psstObj = JSON.parse(window.parent.localStorage.getItem(PSST_LOCALSTORAGE_KEY))
  console.log(`[PSST] #100 PSST_INITIAL_EXECUTION_FLAG:${PSST_INITIAL_EXECUTION_FLAG} \nPSST_CHECK_SETTINGS_LOADED:${PSST_CHECK_SETTINGS_LOADED} \npsst:${JSON.stringify(psstObj)}`)
  if (!psstObj || PSST_INITIAL_EXECUTION_FLAG) {
    clearPolicyResults()
    // Start applying-policy
    const [psstObj, nextUrl] = start()
    console.log(`[PSST] #130 psstObj:${JSON.stringify(psstObj)}`)
    console.log(`[PSST] #130 nextUrl:${nextUrl}`)
    setResultToWindow(getResult(false, psstObj, nextUrl))
    save(psstObj)
    return
  }

  if (psstObj.state === psstState.COMPLETED) {
    setResultToWindow(getResult(true, psstObj, null))
    return
  }

  
  try{
    await waitForCheckboxToLoadWithTimeout(true /* turnOff */)
    moveCurrentTask(psstObj, null)
  } catch (error) {
    console.error("[PSST] Error waiting for checkbox:", error)
    moveCurrentTask(psstObj, error)
  }

  const next_task = psstObj.tasks_list.shift()
  let nextUrl = null
  if (!next_task) {
    psstObj.state = psstState.COMPLETED
    nextUrl = psstObj.start_url
  } else {
    nextUrl = next_task.url
  }

  psstObj.current_task = next_task
  psstObj.progress = calculateProgress(psstObj)

  setResultToWindow(getResult(false, psstObj, nextUrl))
  save(psstObj)
  return
})()
