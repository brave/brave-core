# Reward Confirmation Token

Create an anonymous [confirmation token](https://github.com/brave/brave-browser/wiki/Security-and-privacy-model-for-ad-confirmations).

See [confirmation redemption](../../utility/redeem_confirmation/README.md).

### Payload

| key  | format  | example  | note  |
| -----------  | ------  | -------  | ----  |
| creativeInstanceId  | UUID  | e4958d00-e35c-4134-a408-1fbcf274d5ae  | An id that references the specific ad creative that the user engaged with. This will be the same for any user thatxwxzw engages with this ad.  |
| transactionId  | UUID  | c14d370c-1178-4c73-8385-1cfa17200646  | A unique id for the event that is not re-used. This cannot be linked between ad events or users.  |
| type  | - view<br>- click<br>- landed<br>- conversion<br>- media_play[^1]<br>- media_25[^1]<br>- media_100[^1]<br>- upvote<br>- downvote<br>- flag<br>- bookmark &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;| {"type":"view"}  |  |
| confirmation token  |  | {"blindedPaymentTokens": ["Ev5JE4/9TZI/5TqyN9JWfJ1To0HBwQw2rWeAPcdjX3Q="], "publicKey": "RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="}  | **[TODO: Link to document fragment]**  |
| user data  |  | **[TODO: Add example]**  | See [README.md](../../user_data/README.md#brave-rewards-users)  |

Please add to it!

[^1]: MVP for iOS in Japan. [TODO: Reword]
