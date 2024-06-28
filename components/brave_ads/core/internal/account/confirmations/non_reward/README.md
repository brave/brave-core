# Non-Reward Confirmation

Create an anonymous confirmation.

See [confirmation redemption](../../utility/redeem_confirmation/README.md).

### Payload

| key  | format  | example  | note  |
| -----------  | ------  | -------  | ----  |
| creativeInstanceId  | UUID  | e4958d00-e35c-4134-a408-1fbcf274d5ae  | An id that references the specific ad creative that the user engaged with. This will be the same for any user that engages with this ad.  |
| transactionId  | UUID | c14d370c-1178-4c73-8385-1cfa17200646  | A unique id for the event that is not re-used. This cannot be linked between ad events or users.  |
| type  | Brave Search ads:<br>- conversion<br><br>Brave News ads:<br>- view<br>- click<br>- landed<br>- conversion  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;| {"type":"view"}  |  |
| user data  |  | **[TODO: Add example]**  | See [README.md](../../user_data/README.md#non-brave-rewards-users)  |

Please add to it!
