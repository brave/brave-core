use core::iter;
use std::collections::HashMap;

use challenge_bypass_ristretto::voprf::*;

use chrono::NaiveDate;
use futures_retry::FutureRetry;
use rand::rngs::OsRng;
use serde::{Deserialize, Serialize};
use sha2::Sha512;
use tracing::instrument;

use crate::errors::{InternalError, SkusError};
use crate::http::HttpHandler;
use crate::models::*;
use crate::sdk::SDK;
use crate::{HTTPClient, StorageClient};

#[derive(Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
struct ItemCredentialsRequest {
    item_id: String,
    blinded_creds: Vec<BlindedToken>,
}

#[derive(Debug, Serialize, Deserialize)]
#[serde(untagged)]
enum ItemCredentialsResponse {
    #[serde(rename_all = "camelCase")]
    SingleUse {
        id: String,
        order_id: String,
        blinded_creds: Vec<BlindedToken>,
        signed_creds: Vec<SignedToken>,
        batch_proof: BatchDLEQProof,
        public_key: PublicKey,
        issuer_id: String,
    },
    // FIXME should we just use full date format over wire?
    #[serde(rename_all = "camelCase")]
    TimeLimited {
        expires_at: String,
        id: String,
        issued_at: String,
        order_id: String,
        token: String,
    },
}

impl<U> SDK<U>
where
    U: HTTPClient + StorageClient,
{
    #[instrument]
    pub async fn submit_order_credentials_to_sign(&self, order_id: &str) -> Result<(), SkusError> {
        let order = match self.client.get_order(order_id).await {
            Ok(Some(order)) => order,
            _ => self.refresh_order(order_id).await?,
        };

        if !order.is_paid() {
            return Err(InternalError::OrderUnpaid.into());
        }

        let mut csprng = OsRng;

        for item in order.items {
            match item.credential_type {
                CredentialType::SingleUse => {
                    let blinded_creds: Vec<BlindedToken> =
                        match self.client.get_single_use_item_creds(&item.id).await? {
                            Some(item_creds) => {
                                item_creds.creds.iter().map(|t| t.blind()).collect()
                            }
                            None => {
                                let creds: Vec<Token> =
                                    iter::repeat_with(|| Token::random::<Sha512, _>(&mut csprng))
                                        .take(item.quantity as usize)
                                        .collect();

                                let blinded_creds: Vec<BlindedToken> =
                                    creds.iter().map(|t| t.blind()).collect();

                                self.client
                                    .init_single_use_item_creds(&item.id, creds)
                                    .await?;
                                blinded_creds
                            }
                        };

                    let claim_req = ItemCredentialsRequest {
                        item_id: item.id,
                        blinded_creds,
                    };

                    let request_with_retries = FutureRetry::new(
                        || async {
                            let body = serde_json::to_vec(&claim_req)
                                .or(Err(InternalError::SerializationFailed))?;

                            let req = http::Request::builder()
                                .method("POST")
                                .uri(format!(
                                    "{}/v1/orders/{}/credentials",
                                    self.base_url, order_id
                                ))
                                .body(body)
                                .unwrap();
                            let resp = self.fetch(req).await?;

                            match resp.status() {
                                http::StatusCode::OK => Ok(()),
                                http::StatusCode::CONFLICT => {
                                    Err(InternalError::BadRequest(http::StatusCode::CONFLICT))
                                }
                                http::StatusCode::NOT_FOUND => Err(InternalError::NotFound),
                                _ => Err(resp.into()),
                            }
                        },
                        HttpHandler::new(3, "Sign order item credentials request", &self.client),
                    );
                    request_with_retries.await?;
                }
                CredentialType::TimeLimited => (), // Time limited credentials do not require a submission step
            }
        }
        Ok(())
    }

    #[instrument]
    pub async fn refresh_order_credentials(&self, order_id: &str) -> Result<(), SkusError> {
        let order = self.fetch_order(order_id).await?;
        if let Some(local_order) = self.client.get_order(order_id).await? {
            if order.last_paid_at != local_order.last_paid_at {
                self.fetch_order_credentials(order_id).await?;
                // store the latest retrieved order information after we've successfully fetched
                self.client.upsert_order(&order).await?;
            }
            Ok(())
        } else {
            Err(InternalError::NotFound.into())
        }
    }

    #[instrument]
    pub async fn fetch_order_credentials(&self, order_id: &str) -> Result<(), SkusError> {
        self.submit_order_credentials_to_sign(order_id).await?;

        let request_with_retries = FutureRetry::new(
            || async move {
                let mut builder = http::Request::builder();
                builder.method("GET");
                builder.uri(format!(
                    "{}/v1/orders/{}/credentials",
                    self.base_url, order_id
                ));

                let req = builder.body(vec![]).unwrap();

                let resp = self.fetch(req).await?;

                match resp.status() {
                    http::StatusCode::OK => Ok(resp),
                    http::StatusCode::ACCEPTED => Err(InternalError::RetryLater(None)),
                    http::StatusCode::NOT_FOUND => Err(InternalError::NotFound),
                    _ => Err(resp.into()),
                }
            },
            HttpHandler::new(3, "Fetch order credentials request", &self.client),
        );

        let (resp, _) = request_with_retries.await?;

        let resp: Vec<ItemCredentialsResponse> = serde_json::from_slice(resp.body())?;

        let mut time_limited_creds: HashMap<String, Vec<TimeLimitedCredential>> = HashMap::new();

        for item_cred in resp {
            match item_cred {
                ItemCredentialsResponse::SingleUse {
                    id: item_id,
                    signed_creds,
                    batch_proof,
                    public_key,
                    issuer_id,
                    ..
                } => {
                    if let Some(item_creds) =
                        self.client.get_single_use_item_creds(&item_id).await?
                    {
                        // Rederive blinded creds so that the proof will fail if different creds were signed
                        let blinded_creds: Vec<BlindedToken> =
                            item_creds.creds.iter().map(|t| t.blind()).collect();

                        let unblinded_creds = batch_proof
                            .verify_and_unblind::<Sha512, _>(
                                &item_creds.creds,
                                &blinded_creds,
                                &signed_creds,
                                &public_key,
                            )
                            .or(Err(InternalError::InvalidProof))?;

                        self.client
                            .complete_single_use_item_creds(&item_id, &issuer_id, unblinded_creds)
                            .await?;
                    }
                }
                ItemCredentialsResponse::TimeLimited {
                    id: item_id,
                    order_id: _,
                    issued_at,
                    expires_at,
                    token,
                } => {
                    let cred = TimeLimitedCredential {
                        item_id: item_id.clone(),
                        issued_at: NaiveDate::parse_from_str(&issued_at, "%Y-%m-%d")
                            .map_err(|_| {
                                InternalError::InvalidResponse(
                                    "Could not parse issued at".to_string(),
                                )
                            })?
                            .and_hms(0, 0, 0),
                        expires_at: NaiveDate::parse_from_str(&expires_at, "%Y-%m-%d")
                            .map_err(|_| {
                                InternalError::InvalidResponse(
                                    "Could not parse expires at".to_string(),
                                )
                            })?
                            .and_hms(0, 0, 0),
                        token,
                    };
                    if let Some(item_creds) = time_limited_creds.get_mut(&item_id) {
                        item_creds.push(cred);
                    } else {
                        time_limited_creds.insert(item_id, vec![cred]);
                    }
                }
            }
        }

        for (item_id, item_creds) in time_limited_creds.into_iter() {
            self.client
                .store_time_limited_creds(&item_id, item_creds)
                .await?;
        }

        Ok(())
    }
}
