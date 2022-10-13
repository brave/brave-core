# SKUs SDK

## Time Limited V2 Credentials

### Storage

Time limited V2 credentials are a hybrid of single-use credentials and time limited "shared password"
type credentials.  These time limited credentials are bucketed single-use credentials which have 
been signed with a particular issuer key that is only valid from the valid_from to the valid_to time
period.

In local storage this is the structure of signed credentials:

```
credentials:
	items:
		<item_id>:
			creds:
				0:
					issuer_id: <issuer_id>
					unblinded_creds:
						0:
							spent: false
							unblinded_cred: <unblinded_cred>
					valid_from: "2022-11-16T12:50:26"
					valid_to: "2022-11-17T12:50:26"
```

As seen above each item credential consists of a valid_from, valid_to, issuer_id
as well as a list of unblinded privacy pass protocol tokens with an indicator as
to if it has been spent or not.  Depending on the SKU the order was derived on
the number of unblinded credentials per validity period can vary.  This variation
is primarily bound by the buffer/overlap configuration of the challenge bypass
issuer which is performing the signing process for each time slot.

In order to get the above credentials signed via the privacy pass protocol, the
SKU SDK first creates in local storage a tokens array which includes an appropriate
number of tokens to be signed by the issuer, and assigns that into a datastructure:

```
credentials:
	items:
		8eb63d97-0ad0-4b1a-b696-2769a780dcf5:
			tokens:
				0:
					<unblinded_token>
				...
```

After the SKU SDK generates these clients, it will perform a sign request to the SKUs API and
include the full list of generated tokens to be signed.  The SKUs API will respond with a bucketed
list of credentials, bucketed on time validity as shown below:

```
[
...
  {
    "orderId": "<order_id>",
    "itemId": "<order_item_id>",
    "issuerId": "<privacy_pass_issuer_id>",
    "validTo": "2022-10-18T12:50:26Z",
    "validFrom": "2022-10-17T12:50:26Z",
    "blindedCreds": [
      "<blinded creds>" // derived by the SKU SDK from unblinded creds
    ],
    "signedCreds": [
      "<signed creds>" // derived by the Challenge Bypass Server
    ],
    "batchProof": "<dleq batch proof>",
    "publicKey": "<issuer public key>"
  }
]
```

Upon receiving these signed credentials async, the SKU SDK iterates through the signed credentials,
storing them in local storage, as well as storing the original unblinded tokens from the `tokens` array
along side the signed credentials, so that they can be used for presentation in the present process.

After the signed credentials are associated with the unblinded credentials, the `tokens` array is emptied.
Upon a future date when the credentials need to be replenished, the process works the same way, the tokens
array is populated with unblinded tokens, submitted for signing, resulting signed credentials are associated with
the unblinded tokens and are stored, and the originally generated unblinded tokens array is emptied.



