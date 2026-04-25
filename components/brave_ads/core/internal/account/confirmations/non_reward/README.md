# Non-Brave Rewards User Confirmation Payload

Return to [non-Brave Rewards user confirmation redemption](../../utility/redeem_confirmation/non_reward/README.md).

Included when redeeming an anonymous confirmation:

| key  | example  | description  |
| ---  | ---  | ---  |
| creativeInstanceId  | {"creativeInstanceId":"e4958d00-e35c-4134-a408-1fbcf274d5ae"}  | An id that references the specific ad creative that the user engaged with. This will be the same for any user that engages with this ad.  |
| firstTime  | {"firstTime":true}  | Indicates whether this is the first time a browser profile has reported a metric for a specific campaign.  |
| type  | {"type":"click"}  | Action or interaction that occurred within an advertisement, such as a user clicking the ad.<br><br>Supported types:<br><br>Brave New Tab Page ads:<br>- view<br>- click<br>- interaction<br>- media_play<br>- media_25<br>- media_100<br><br>Brave Search ads:<br>- conversion<br>|
| user data  | {"conversion":[{"action":"click"}]}  | See [user data](../../user_data/README.md#non-brave-rewards-user).  |

Please add to it!
