# Brave Rewards User Payment Token Redemption

Return to [Brave Rewards user confirmation token redemption](../../../account/utility/redeem_confirmation/reward/README.md).

Request payment for all user-collected Brave Rewards, by calling https://mywallet.ads.brave.com/v{version}/confirmation/payment/{paymentId}.

Here, `{version}` represents the version number of the targeted API, and `{paymentId}` is the Rewards payment id, which can be found at [brave://rewards-internals](brave://rewards-internals).

The request body includes the following:

| key  | example  | description  |
|---|---|---|
| payload   | {"paymentId":"27a39b2f-9b2e-4eb0-bbb2-2f84447496e7"}  | Rewards payment id, which can be found at [brave://rewards-internals](brave://rewards-internals).  |
| paymentCredentials  | {"paymentCredentials":[<br>{"confirmationType":"view","credential":{"signature":"H9HPNdEVJBvc9d8RZig/Gihlrcgug/n/rRaAJzeZI20gKPCivIj9Ig8StvqMSc5GfgLrBaJDibwBghnhRhqYRQ==",<br>"t":"PLowz2WF2eGD5zfwZjk9p76HXBLDKMq/3EAZHeG/fE2XGQ48jyte+Ve50ZlasOuYL5mwA8CU2aFMlJrt3DDgCw=="},<br>"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},<br>{"confirmationType":"click","credential":{"signature":"mfv+HJP5K/q9ogcGwD4uqOd98sb2fx96h+QnsdtGwJ4wdZfvrukbP4whyz46Ro3gm2FIMhPWZ5wM2Hhg9OGPtg==",<br>"t":"hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6NgXPHUeyaxzd6/Lk6YHlfQ=="},<br>"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="}]}  | A collection of payment credentials corresponding to each payment token.  |
| platform  | {"platform": "android"}  | Operating system. See [user data](../../user_data/README.md#user-data-submitted-when-redeeming-identifiable-payment-tokens).  |
| summary  | {"totals":[{"ad_format":"ad_notification","view":1,"click":1}]}  | Summary of payment tokens for each creative. See [user data](../../user_data/README.md#user-data-submitted-when-redeeming-identifiable-payment-tokens).  |

Please add to it!
