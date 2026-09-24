# Brave Rewards User Confirmation Token Payload

Return to [Brave Rewards user confirmation token redemption](../../utility/redeem_confirmation/reward/README.md).

Included when redeeming an anonymous confirmation token:

| key  | example  | description  |
| ---  | ---  | ---  |
| creativeInstanceId  | {"creativeInstanceId":"e4958d00-e35c-4134-a408-1fbcf274d5ae"}  | An id that references the specific ad creative that the user engaged with. This will be the same for any user that engages with this ad.  |
| transactionId  | {"transactionId":"c14d370c-1178-4c73-8385-1cfa17200646"}  | A unique id for the transaction, which is not linkable between confirmation token redemptions.  |
| firstTime  | {"firstTime":true}  | Indicates whether this is the first time a browser profile has reported a metric for a specific campaign.  |
| type  | {"type":"view"}  | Action or interaction that occurred within an advertisement, such as a user clicking the ad.<br><br>Supported types:<br><br>- view<br>- click<br>- landed<br>- conversion<br>- interaction[^1]<br>- media_play[^1]<br>- media_25[^1]<br>- media_100[^1]<br>- upvote<br>- downvote<br>- flag<br>- bookmark &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  |
| confirmation token  | {"blindedPaymentTokens": ["+qJiMi6k0hRzRAEN239nLthLqrNm53O78x/PV8I/JS0="],<br> "publicKey": "OqhZpUC8B15u+Gc11rQYRl8O3zOSAUIEC2JuDHI32TM="}  | See [security and privacy model for ad confirmations](https://github.com/brave/brave-browser/wiki/Security-and-privacy-model-for-ad-confirmations).  |
| user data  | {"buildChannel":"nightly",<br>"catalog":[{"id":"29e5c8bc0ba319069980bb390d8e8f9b58c05a20"}],<br>"conversion":[{"action":"view"}],<br>"createdAtTimestamp":"2020-11-18T12:00:00.000Z",<br>"httpResponseStatus":"500",<br>"platform":"windows",<br>"rotatingHash":"I6KM54gXOrWqRHyrD518LmhePLHpIk4KSgCKOl0e3sc=",<br>"segment":"sports",<br>"studies":[{"group":"GroupA","name":"BraveAds.FooStudy"},{"group":"GroupB","name":"BraveAds.BarStudy"}],<br>"versionNumber":"1.2.3.4"}  | See [user data](../../user_data/README.md#brave-rewards-users).  |

Please add to it!

[^1]: New tab page ads
