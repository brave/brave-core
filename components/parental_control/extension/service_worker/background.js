import {defaultBlockRules} from "../assets/rules/defaultBlockRules.js";
import {socialMediaBlockRules} from "../assets/rules/socialMediaBlockRules.js";
import {gamingSiteRules} from "../assets/rules/gamesBlockRules.js";

let startTime;
let timerId;
let timeoutDuration;
const startSessionTimeout = () => {
    startTime = Date.now();
    timerId = setTimeout(sessionTimeout, timeoutDuration);
    console.log(timeoutDuration)
}

// Function to update time in local storage every minute
const updateTimeInLocalStorage = () => {
    setInterval(() => {
        const currentTime = Date.now();
        // console.log('Current time:', currentTime);
        chrome.storage.local.set({ timeLeft: timeoutDuration -  (currentTime - startTime) })
    }, 1000); // 60000 milliseconds = 1 minute
}

updateTimeInLocalStorage();

// Function to restart the timer with the remaining time when the first window is opened again
const restartTimer = async () => {
    // clearTimeout(timerId);
    console.log("restart timer")

    await new Promise((resolve, reject) => {
        chrome.storage.local.get(['timeLeft'], (data) => {
            if (chrome.runtime.lastError) {
                reject(chrome.runtime.lastError);
            } else {
                resolve(data);
            }

            timeoutDuration = data.timeLeft;
            console.log(timeoutDuration, " timeoutduration")
        });
    });
    startSessionTimeout()
}


// Listener for when a window is created
chrome.windows.onCreated.addListener(async () =>{
    const data = await new Promise((resolve, reject) => {
        chrome.storage.local.get(['loggedIn', 'sessionTimeout','timeLeft'], (data) => {
            if (chrome.runtime.lastError) {
                reject(chrome.runtime.lastError);
            } else {
                resolve(data);
            }
        });
    });
    chrome.windows.getAll({ populate: false }, (windows) => {
        if (windows.length === 1) {
            if(data.loggedIn && data.sessionTimeout != true)
            {restartTimer();}
            // console.log(data.loggedIn, data.sessionTimeout, data.timeLeft)
            console.log('First window opened!');
        }
    });
});

chrome.tabs.onCreated.addListener((tab) => {
    chrome.storage.local.get(['loggedIn', 'sessionTimeout'], (data) => {
        if (data.loggedIn && data.sessionTimeout) {
            const tabUrl = tab.url;
            if (tabUrl !== "chrome://extensions/") {

                chrome.tabs.update(tab.id, { url: '../content/ui/sessionTimeout.html' });
            }
            blockHttpsSearch();
        }
    });
});

const sessionTimeout = () => {

    // Perform necessary actions when the session times out
    chrome.storage.local.set({ sessionTimeout: true }, () => {
        if (chrome.runtime.lastError) {
            console.error('Error setting sessionTimeout flag:', chrome.runtime.lastError);
        }
    });
    console.log("sessionTimeout")
    chrome.windows.create({
        url: '../content/ui/sessionTimeout.html',
        type: 'normal'
    },
        chrome.windows.getAll({ populate: true }, (windows) => {
            windows.forEach((window) => {
                chrome.windows.remove(window.id);
            });
        })
    )
}

// Function to block Google search URLs
const blockHttpsSearch = () => {
    const blockRule = {
        id: 1,
        priority: 1,
        action: {
            type: 'block'
        },
        condition: {
            urlFilter: 'https://*/*',
            resourceTypes: ['main_frame']
        }
    };

    chrome.declarativeNetRequest.updateDynamicRules({
        removeRuleIds: [],
        addRules: [blockRule]
    });
}

// Function to allow Google search URLs
const allowHttpsSearchAsync = async () => {
    return new Promise((resolve, reject) => {
        chrome.declarativeNetRequest.updateDynamicRules({
            removeRuleIds: [1],
            addRules: []
        }, () => {
            if (chrome.runtime.lastError) {
                reject(chrome.runtime.lastError);
            } else {
                resolve();
            }
        });
    });
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

// Function to start kids mode
const kidsModeSignUp = async (cpassword, password, sendResponse) => {
    if (cpassword === password) {
        try {
            const hash = await hashPassword(password)
            chrome.storage.local.set({ loggedIn: false, password: hash }, () => {
                if (chrome.runtime.lastError) {
                    console.error('Error storing data:', chrome.runtime.lastError);
                    sendResponse({ success: false });
                } else if (cpassword && password) {
                    console.log('Kids mode started successfully:', cpassword);
                    sendResponse({ success: true });
                }
            });
        }
        catch (error) {
            console.error('Error hashing password:', error);
        }
    }
    else {
        // Confirm Password mismatch, send error response
        sendResponse({ success: false, error: '*Passwords do not match' });
    }
}

const kidsModeSignIn = async (password, sendResponse) => {
    try {
        const data = await new Promise((resolve, reject) => {
            chrome.storage.local.get(['loggedIn', 'password'], (data) => {
                if (chrome.runtime.lastError) {
                    reject(chrome.runtime.lastError);
                } else {
                    resolve(data);
                }
            });
        });

        const storedPassword = data.password;

        const hash = await hashPassword(password);

        if (storedPassword === hash) {
            
            try {
                timeoutDuration = 1 * 60 * 1000
                startSessionTimeout();
                chrome.storage.local.set({ loggedIn: true }, () => {
                    if (chrome.runtime.lastError) {
                        console.error('Error storing data:', chrome.runtime.lastError);
                        sendResponse({ success: false });
                    } else  {
                        console.log('Kids mode started successfully:');
                        sendResponse({ success: true });
    
                        // Close current windows
                        chrome.windows.getAll({ populate: true }, (windows) => {
                            windows.forEach((window) => {
                                chrome.windows.remove(window.id);
                            });
                        });
    
                        // Create a new window with Google
                        chrome.windows.create({
                            url: '../content/ui/home.html',
                            type: 'normal'
                        });
                    }
                });
            }
            catch (error) {
                console.error('Error logging in:', error);
            }}
            else {
                // Password mismatch, send error response
                sendResponse({ success: false, error: '*Invalid credentials' });
            }
        }catch(error){
            console.error('Error logging in:', error);
        }
}
// Function to handle user logout
const logoutUser = async (password, sendResponse) => {
    try {
        const data = await new Promise((resolve, reject) => {
            chrome.storage.local.get(['loggedIn', 'password'], (data) => {
                if (chrome.runtime.lastError) {
                    reject(chrome.runtime.lastError);
                } else {
                    resolve(data);
                }
            });
        });

        const storedPassword = data.password;

        const hash = await hashPassword(password);

        // Check if the provided password matches the stored password
        if (storedPassword === hash) {
            removeServiceWorker();
            clearTimeout(timerId);
            chrome.storage.local.set({loggedIn : false, sessionTimeout:false, timeLeft: 1 * 60 * 1000},)

            console.log('User logged out successfully');
            sendResponse({ success: true });

            await allowHttpsSearchAsync();

            chrome.windows.getAll({ populate: true }, (windows) => {
                windows.forEach((window) => {
                    chrome.windows.remove(window.id);
                });
            });

            chrome.windows.create({
                url: 'https://google.com',
                type: 'normal'
            });
        } else {
            sendResponse({ success: false, error: '*Invalid Credentials' });
        }
    } catch (error) {
        console.error('Error logging out:', error);
        sendResponse({ success: false, error: 'An error occurred while logging out' });
    }
}

export const injectServiceWorker = async (checkedToggles) => {
    const rulesToInject = [];
    const oldRules = await chrome.declarativeNetRequest.getDynamicRules();
    const oldRulesIds = oldRules.map(rule => rule.id);

    rulesToInject.push(...defaultBlockRules);

    checkedToggles.forEach((toggle) => {
        const {id}= toggle;
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

    await chrome.declarativeNetRequest.updateDynamicRules({
        removeRuleIds: oldRulesIds,
        addRules: rulesToInject
    });
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

    }else if (request.action === 'login') {
        kidsModeSignIn(request.password, sendResponse);
        return true;
    } else if (request.action === 'logout') {
        logoutUser(request.password, sendResponse);
        return true;
    }
});
