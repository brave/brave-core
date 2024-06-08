import { defaultBlockRules } from "../assets/rules/defaultBlockRules.js";
import { socialMediaBlockRules } from "../assets/rules/socialMediaBlockRules.js";
import { gamingSiteRules } from "../assets/rules/gamesBlockRules.js";

const LOCAL_STORAGE_UPDATE_INTERVAL = 2e4;

let timerId;
// Function to restart the timer with the remaining time when the first window is opened again
const startTimer = async () => {
    try {
        const data = await chrome.storage.local.get(['timeLeft', 'loggedIn', 'sessionTimeout'])
        if (!data.loggedIn || data.sessionTimeout) return;
        if (data.timeLeft) {
            timerId = setTimeout(sessionTimeout, data.timeLeft);
            updateTimeInLocalStorage(data.timeLeft);
        }
    } catch (error) {
        console.error("error starting timer ", error)
    }
}

chrome.tabs.onCreated.addListener(async (tab) => {
    const data = await chrome.storage.local.get(['loggedIn', 'sessionTimeout'])
    if (data.loggedIn) {
        chrome.action.setIcon({ path: "../assets/Logo_active.png" });
    }
    if (data.loggedIn && data.sessionTimeout) {
        chrome.tabs.update(tab.id, { url: '../content/ui/sessionTimeout.html' });
    }
    else if (data.loggedIn && !data.sessionTimeout) {
        const tabs = await chrome.tabs.query({})
        if (tabs.length === 1) {
            startTimer();
        }
    }
});

// Function to update time in local storage every minute
let intervalId;
const updateTimeInLocalStorage = async (timeLeft) => {
    intervalId = setInterval(async () => {
        timeLeft -= LOCAL_STORAGE_UPDATE_INTERVAL;
        await chrome.storage.local.set({ timeLeft: timeLeft })
        if (timeLeft < 0) {
            clearInterval(intervalId)
        }
    }, LOCAL_STORAGE_UPDATE_INTERVAL); // 20000 milliseconds = 20 sec
}

const sessionTimeout = async () => {
    await blockHttpsSearch();
    await chrome.storage.local.set({ sessionTimeout: true });
    const url = '../content/ui/sessionTimeout.html';
    await handleBrowserWindows(url);
}

// Function to block Google search URLs
const blockHttpsSearch = async () => {
    const blockRule = {
        id: 999,
        priority: 1,
        action: {
            type: 'block'
        },
        condition: {
            urlFilter: 'https://*/*',
            resourceTypes: ['main_frame']
        }
    };

    await chrome.declarativeNetRequest.updateDynamicRules({
        removeRuleIds: [],
        addRules: [blockRule]
    });
}

// Function to allow Google search URLs
const allowHttpsSearchAsync = async () => {
    await chrome.declarativeNetRequest.updateDynamicRules({
        removeRuleIds: [999],
        addRules: []
    })
}

// Hash password function
const hashPassword = async (password) => {
    const encoder = new TextEncoder();
    const data = encoder.encode(password);

    const hashBuffer = await crypto.subtle.digest('SHA-256', data);
    const hashArray = Array.from(new Uint8Array(hashBuffer));
    const hashHex = hashArray.map(byte => byte.toString(16).padStart(2, '0')).join('');

    return hashHex;
}

const kidsModeSignUp = async (cpassword, password, sendResponse) => {

    if (!cpassword || !password)
        return sendResponse({ status: false, error: "Please enter a password" });

    if (cpassword !== password)
        return sendResponse({ status: false, error: "Passwords do not match" });

    try {
        const hash = await hashPassword(password)
        await chrome.storage.local.set({ loggedIn: false, password: hash })
        sendResponse({ status: true })
    } catch (error) {
        sendResponse({ status: false, error: "Error setting password" });
    }
}

const kidsModeSignIn = async (password, checkedToggles, sessionTime, sendResponse) => {
    try {
        const data = await chrome.storage.local.get(['password']);
        const storedHash = data.password;
        const hash = await hashPassword(password);

        if (!password)
            return sendResponse({ status: false, error: "Please enter a password" });

        if (storedHash !== hash)
            return sendResponse({ status: false, error: "Wrong password" });

        try {
            await injectServiceWorker(checkedToggles)
            const timeoutDuration = sessionTime * 60 * 60 * 1000; //no. of hrs * 60 min
            await chrome.storage.local.set({ loggedIn: true, timeLeft: timeoutDuration, sessionTimeout: false })
            sendResponse({ status: true });
            await handleBrowserWindows();
        } catch (error) {
            sendResponse({ status: false, error: "Error logging in, try closing all the windows" });
        }
    } catch (error) {
        sendResponse({ status: false, error: "An error occurred while logging in" });
    }
}

const logoutUser = async (password, sendResponse) => {
    try {
        const data = await chrome.storage.local.get(['loggedIn', 'password'])
        const storedPassword = data.password;
        const hash = await hashPassword(password);

        if (storedPassword !== hash)
            return sendResponse({ status: false, error: "Wrong password" });

        try {
            chrome.action.setIcon({ path: "../assets/Logo_inactive.png" });
            removeServiceWorker();
            clearTimeout(timerId);
            clearInterval(intervalId)
            await chrome.storage.local.set({ loggedIn: false, sessionTimeout: false })
            sendResponse({ status: true });
            await allowHttpsSearchAsync();
            await handleBrowserWindows();
        } catch (error) {
            sendResponse({ status: false, error: "Error logging out, try closing all the windows" })
        }
    } catch (error) {
        sendResponse({ status: false, error: "An error occurred while logging out" });
    }
}

const handleBrowserWindows = async (url) => {
    const windows = await chrome.windows.getAll({ populate: true })
    windows.forEach((window) => {
        chrome.windows.remove(window.id);
    });
    if (!url) await chrome.windows.create({ type: 'normal' });
    else chrome.windows.create({ url: url, type: 'normal' });
}

const updateBlockingRules = async (rulesToInject) => {
    const oldRules = await chrome.declarativeNetRequest.getDynamicRules();
    const oldRulesIds = oldRules.map(rule => rule.id);

    await chrome.declarativeNetRequest.updateDynamicRules({
        removeRuleIds: oldRulesIds,
        addRules: rulesToInject
    });
}

export const injectServiceWorker = async (checkedToggles) => {

    if (!checkedToggles) return;
    const rulesToInject = [];
    rulesToInject.push(...defaultBlockRules);

    checkedToggles.forEach((toggle) => {
        const { id } = toggle;
        switch (id) {
            case "socialMediaToggle":
                rulesToInject.push(...socialMediaBlockRules);
                break;
            case "gamingToggle":
                rulesToInject.push(...gamingSiteRules);
                break;
            default:
                console.log("Unknown toggle status");
        }
    });

    updateBlockingRules(rulesToInject);
}

//remove all the rules on the session end
const removeServiceWorker = async () => {
    const oldRules = await chrome.declarativeNetRequest.getDynamicRules();
    const oldRulesIds = oldRules.map(rule => rule.id);
    await chrome.declarativeNetRequest.updateDynamicRules({
        removeRuleIds: oldRulesIds
    });
}


// Listener for messages from content scripts or UI components
chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
    if (request.action === 'register') {
        kidsModeSignUp(request.cpassword, request.password, sendResponse);
        return true;
    } else if (request.action === 'login') {
        kidsModeSignIn(request.password, request.checkedToggles, request.sessionTime, sendResponse);
        return true;
    } else if (request.action === 'logout') {
        logoutUser(request.password, sendResponse);
        return true;
    }
});