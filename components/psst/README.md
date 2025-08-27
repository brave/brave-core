# Privacy Settings Selection for Sites Tool (PSST)

The feature helps users apply the best practices for selecting privacy settings.

## Spec
https://docs.google.com/document/d/1ccnBWBV_KkknZpZYxcOXTwtfIiaSzeLS5dMd08N2pbs/edit?tab=t.0


# PSST CRX Component

Contains set of rules and scripts for small number of very popular sites (Google, Facebook, Twitter, Twitch, etc.). 
Each rule set would be managed in open source (in or similar to https://github.com/brave/adblock-lists), and shipped daily to users with a separate component.

Component ID: `lhhcaamjbmbijmjbnnodjaknblkiagon`
Component version: `1`

### Component's folder structure:

```
<component id>/<component version>/
 |_ manifest.json
 |_ psst.json
 |_ scripts/
    |_ twitter/
        |_ user.js
        |_ policy.js
    |_ linkedin/
        |_ user.js
        |_ policy.js
```

#### psst.json

Contains a set of rules for each supported website:

Example:
```
[
    {
        "name": "twitter",
        "include": [
            "https://x.com/*"
        ],
        "exclude": [
        ],
        "version": 4,
        "user_script": "user.js",
        "policy_script": "policy.js"
    },
    {
        "name": "linkedin",
        "include": [
            "https://www.linkedin.com/*"
        ],
        "exclude": [
        ],
        "version": 1,
        "user_script": "user.js",
        "policy_script": "policy.js"
    }
]
```

#### user.js

JS script which helps to detect that the user is signed in on the current website, which means we are ready to apply privacy settings.
The result of user script execution is JSON, which contains the next fields:
`user` - contains the identifier of the signed user for the current website.
`tasks` - list of objects ( url and description pairs ) where url is the address of the psst settings we propose to change and its description

Example:
```
 {
  "user": <signed user identifier>,
  "tasks": {
      {
        url:<setting url, MUST BE UNIQUE>,
        description:<setting description>,
      },
       .... 
   }
 }
```

#### privacy.js

Privacy script takes as parameters the tasks list from the user script output and saves it to the local storage. When the policy script is executed and local storage already contains the tasks list it takes one task and processes it.
To pass parameter to the policy script we should prepend the script with the definition of the params variable, which contains tasks, returned by user script: 

##### Policy script parameters:

```
const params = {
   "tasks": [ {
      "description": "Ads Preferences",
      "url": "https://x.com/settings/ads_preferences"
   } ]
}
;
<policy script content>

```

