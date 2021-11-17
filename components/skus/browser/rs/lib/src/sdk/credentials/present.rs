use chrono::Utc;
use hmac::Hmac;
use http::uri;
use serde::{Deserialize, Serialize};
use serde_json::json;
use sha2::Sha512;
use tracing::instrument;

use crate::errors::{InternalError, SkusError};
use crate::models::*;
use crate::sdk::SDK;
use crate::{HTTPClient, StorageClient};

type HmacSha512 = Hmac<Sha512>;

#[derive(Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
struct VerifyCredentialRequest {
    #[serde(rename = "type")]
    credential_type: CredentialType,
    version: u8,
    sku: String,
    presentation: String,
}

impl<U> SDK<U>
where
    U: HTTPClient + StorageClient,
{
    pub fn encode_issuer_id(&self, merchant: &str, sku: &str) -> Result<String, InternalError> {
        let query = "?sku=".to_string() + sku;
        // Validate merchant and sku
        let uri = uri::Builder::new()
            .scheme("")
            .authority(merchant)
            .path_and_query(query.as_str())
            .build();
        match uri {
            // Work around required scheme
            Ok(_) => Ok(format!("{}{}", merchant, query)),
            Err(_) => Err(InternalError::InvalidMerchantOrSku),
        }
    }

    #[instrument]
    pub async fn prepare_order_credentials_presentation(
        &self,
        order_id: &str,
        domain: &str,
        path: &str,
    ) -> Result<Option<String>, SkusError> {
        let order = self.client.get_order(order_id).await?;

        if let Some(order) = order {
            if !order.location_matches(&self.environment, domain) {
                return Err(InternalError::OrderLocationMismatch.into());
            }

            // FIXME only returns summary for first item
            if let Some(item) = order.items.into_iter().next() {
                let name = "sku#".to_string() + &item.sku;

                let (expires_at, presentation) = match item.credential_type {
                    CredentialType::SingleUse => {
                        let creds = self
                            .client
                            .get_single_use_item_creds(&item.id)
                            .await?
                            .ok_or(InternalError::ItemCredentialsMissing)?;
                        let unblinded_creds = creds
                            .unblinded_creds
                            .ok_or(InternalError::ItemCredentialsMissing)?;

                        // retrieve the next unspent token
                        let (i, cred) = unblinded_creds
                            .into_iter()
                            .enumerate()
                            .find(|(_i, cred)| !cred.spent)
                            .ok_or(InternalError::OutOfCredentials)?;

                        let issuer = self.encode_issuer_id(&order.merchant_id, &item.sku)?;

                        let verification_key =
                            cred.unblinded_cred.derive_verification_key::<Sha512>();
                        // FIXME change the payload we're creating the binding with
                        let signature = verification_key
                            .sign::<HmacSha512>(issuer.as_bytes())
                            .encode_base64();

                        let redemption = json!({
                            "issuer": issuer,
                            "t": cred.unblinded_cred.t,
                            "signature": signature,
                        });
                        let presentation = base64::encode(&redemption.to_string());

                        self.client.spend_single_use_item_cred(&item.id, i).await?;

                        (None, presentation)
                    }
                    CredentialType::TimeLimited => {
                        let cred = self
                            .matching_time_limited_credential(&item.id)
                            .await?
                            .ok_or(InternalError::ItemCredentialsMissing)?;

                        if Utc::now().naive_utc() > cred.expires_at {
                            return Err(InternalError::ItemCredentialsExpired.into());
                        }

                        let presentation = json!({
                            "issuedAt": cred.issued_at.date().to_string(),
                            "expiresAt": cred.expires_at.date().to_string(),
                            "token": cred.token,
                        });

                        (
                            Some(format!(
                                "{}",
                                cred.expires_at.format("%a, %d %b %Y %H:%M:%S GMT")
                            )),
                            base64::encode(&serde_json::to_vec(&presentation)?),
                        )
                    }
                };

                let payload = urlencoding::encode(&base64::encode(&serde_json::to_vec(
                    &VerifyCredentialRequest {
                        credential_type: item.credential_type,
                        // FIXME
                        version: 1,
                        sku: item.sku.to_string(),
                        presentation,
                    },
                )?));

                let mut value = format!("{}={};path={};samesite=strict", name, &payload, path,);
                if let Some(expires_at) = expires_at {
                    value = format!("{};expires={}", value, expires_at);
                }
                if self.environment != Environment::Local
                    && self.environment != Environment::Testing
                {
                    value = format!("__Secure-{};secure", value);
                }

                // only a single credential is allowed for any given location
                return Ok(Some(value));
            }
        } else {
            return Err(InternalError::NotFound.into());
        }

        Ok(None)
    }

    #[instrument]
    pub async fn prepare_credentials_presentation(
        &self,
        domain: &str,
        path: &str,
    ) -> Result<Option<String>, SkusError> {
        if let Some(orders) = self.client.get_orders().await? {
            for order in orders {
                if order.location_matches(&self.environment, domain) {
                    match self
                        .prepare_order_credentials_presentation(&order.id, domain, path)
                        .await
                    {
                        Ok(Some(value)) => return Ok(Some(value)),
                        Ok(None) => continue,
                        Err(e) => match e.0 {
                            InternalError::ItemCredentialsExpired
                            | InternalError::ItemCredentialsMissing
                            | InternalError::OutOfCredentials
                            | InternalError::UnhandledVariant => continue,
                            e => return Err(e.into()),
                        },
                    };
                }
            }
        }
        Ok(None)
    }

    #[instrument]
    pub async fn present_order_credentials(
        &self,
        order_id: &str,
        domain: &str,
        path: &str,
    ) -> Result<Option<String>, SkusError> {
        if let Some(value) = self
            .prepare_order_credentials_presentation(order_id, domain, path)
            .await?
        {
            // NOTE web server which recieves the cookie should unset it
            self.client.set_cookie(&value);
            return Ok(Some(value));
        }
        Ok(None)
    }

    #[instrument]
    pub async fn present_credentials(
        &self,
        domain: &str,
        path: &str,
    ) -> Result<Option<String>, SkusError> {
        if let Some(value) = self.prepare_credentials_presentation(domain, path).await? {
            // NOTE web server which recieves the cookie should unset it
            self.client.set_cookie(&value);
            return Ok(Some(value));
        }
        Ok(None)
    }
}
