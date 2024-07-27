# Non-Brave Rewards User Confirmation Payload

Return to [non-Brave Rewards user confirmation redemption](../../utility/redeem_confirmation/non_reward/README.md).

Included when redeeming an anonymous confirmation:

| key  | example  | description  |
| ---  | ---  | ---  |
| creativeInstanceId  | {"creativeInstanceId":"e4958d00-e35c-4134-a408-1fbcf274d5ae"}  | An id that references the specific ad creative that the user engaged with. This will be the same for any user that engages with this ad.  |
| transactionId  | {"transactionId":"c14d370c-1178-4c73-8385-1cfa17200646"}  | A unique id for the transaction, which is not linkable between confirmation redemptions.  |
| type  | {"type":"click"}  | Action or interaction that occurred within an advertisement, such as a user clicking the ad.<br><br>Supported types:<br><br>Brave Search ads:<br>- conversion<br><br>Brave News ads:<br>- view<br>- click<br>- landed<br>- conversion &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  |
| user data  | {"conversion":[{"action":"click"}]}  | See [user data](../../user_data/README.md#non-brave-rewards-user).  |

Please add to it!
