// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use core::iter;
use std::collections::HashMap;

use challenge_bypass_ristretto::voprf::*;

use chrono::{NaiveDate, NaiveDateTime, Utc};
use futures_retry::FutureRetry;
use rand::rngs::OsRng;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use sha2::Sha512;
use uuid::Uuid;

use crate::errors::{InternalError, SkusError};
use crate::http::{delay_from_response, HttpHandler};
use crate::models::*;
use crate::sdk::SDK;
use crate::{HTTPClient, StorageClient};
use tracing::{event, instrument, Level};

#[derive(Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
struct ItemCredentialsRequest {
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
    #[serde(rename_all = "camelCase")]
    TimeLimitedV2 {
        item_id: String,
        order_id: String,
        blinded_creds: Vec<BlindedToken>,
        signed_creds: Vec<SignedToken>,
        batch_proof: BatchDLEQProof,
        public_key: PublicKey,
        issuer_id: String,
        valid_from: String,
        valid_to: String,
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
    async fn generate_and_upsert_time_limited_v2_creds(
        &self,
        item_id: &str,
        request_id: &str,
        num_creds: usize,
    ) -> Result<Vec<BlindedToken>, SkusError> {
        let mut csprng = OsRng;

        let creds: Vec<Token> =
            iter::repeat_with(|| Token::random::<Sha512, _>(&mut csprng)).take(num_creds).collect();

        let blinded_creds: Vec<BlindedToken> = creds.iter().map(|t| t.blind()).collect();

        self.client.upsert_time_limited_v2_item_creds(item_id, request_id, creds).await?;

        Ok(blinded_creds)
    }

    #[instrument]
    pub async fn submit_order_item_credentials_to_sign(
        &self,
        order_id: &str,
        item_id: &str,
    ) -> Result<String, SkusError> {
        event!(Level::DEBUG, "submit order creds for signing");

        let mut request_id = Uuid::new_v4().to_string();

        let order = match self.client.get_order(order_id).await {
            Ok(Some(mut order)) => {
                // if the order has no order metadata, attempt to refresh first
                if order.metadata.is_none() {
                    order = self.refresh_order(order_id).await?;
                    event!(Level::DEBUG, order=?order, "fetched order, no metadata");
                }
                order
            }
            _ => self.refresh_order(order_id).await?,
        };

        if !order.can_submit_credentials(Utc::now().naive_utc()) {
            return Err(InternalError::OrderUnpaid.into());
        }

        if let Some(item) = order.items.iter().find(|x| x.id == item_id) {
            match item.credential_type {
                CredentialType::TimeLimitedV2 => {
                    let mut num_creds: usize = 0;
                    if let Some(ref metadata) = order.metadata {
                        let num_intervals =
                            metadata.num_intervals.ok_or(InternalError::OrderMisconfiguration)?;
                        let num_per_interval = metadata
                            .num_per_interval
                            .ok_or(InternalError::OrderMisconfiguration)?;
                        num_creds = num_intervals * num_per_interval;
                    }

                    event!(Level::DEBUG, num_creds=?num_creds, "num_creds");

                    let blinded_creds: Vec<BlindedToken> =
                        match self.client.get_time_limited_v2_creds(&item.id).await? {
                            Some(item_creds) => {
                                // are we almost expired
                                let almost_expired = self
                                    .last_matching_time_limited_v2_credential(&item.id)
                                    .await?
                                    .iter()
                                    .any(|cred| {
                                        Utc::now().naive_utc()
                                            > (cred.valid_to - chrono::Duration::days(5))
                                    });

                                let creds: Vec<BlindedToken> =
                                    item_creds.creds.iter().map(|t| t.blind()).collect();

                                match item_creds.state {
                                    CredentialState::GeneratedCredentials
                                    | CredentialState::SubmittedCredentials => {
                                        // overwrite our request id if creds are already present
                                        // that we need to resubmit
                                        request_id = match item_creds.request_id {
                                            Some(request_id) => request_id,
                                            // use the item id as the request id if it was not persisted
                                            None => item_id.to_string(),
                                        };

                                        // we have generated, or performed submission, reuse the
                                        // creds we created for signing.
                                        creds
                                    }
                                    CredentialState::ActiveCredentials
                                    | CredentialState::FetchedCredentials => {
                                        if almost_expired {
                                            self.generate_and_upsert_time_limited_v2_creds(
                                                &item.id,
                                                &request_id,
                                                num_creds,
                                            )
                                            .await?
                                        } else {
                                            // nothing to submit for signing
                                            Vec::<BlindedToken>::new() // nothing to do
                                        }
                                    }
                                }
                            }
                            None => {
                                // we have no idea about this order item Start State
                                self.generate_and_upsert_time_limited_v2_creds(
                                    &item.id,
                                    &request_id,
                                    num_creds,
                                )
                                .await?
                            }
                        };

                    // if the blinded_creds length is less than 1 we do not want to submit this
                    // request.  this happens if we currently are full with credentials

                    if !blinded_creds.is_empty() {
                        let claim_req = ItemCredentialsRequest { blinded_creds };

                        let request_with_retries = FutureRetry::new(
                            || async {
                                let body = serde_json::to_vec(&claim_req)
                                    .or(Err(InternalError::SerializationFailed))?;

                                let req = http::Request::builder()
                                    .method("PUT")
                                    .uri(format!(
                                        "{}/v1/orders/{}/credentials/items/{}/batches/{}",
                                        self.base_url, order_id, item.id, request_id,
                                    ))
                                    .body(body)?;

                                let resp = self.fetch(req).await?;

                                match resp.status() {
                                    http::StatusCode::OK => Ok(()),
                                    http::StatusCode::CONFLICT => {
                                        // On conflict we need to regenerate our request id since
                                        // this indicates a different set of credentials were
                                        // already submitted for the existing id
                                        // NOTE: this should only happen in one of two cases:
                                        // 1. we upgraded from a browser version that did not
                                        //    persist request ids and there are multiple devices
                                        // 2. we accidentally re-used a request id due to
                                        //    https://github.com/brave/brave-browser/issues/35742
                                        self.client.upsert_time_limited_v2_item_creds_request_id(
                                            &item.id,
                                            &Uuid::new_v4().to_string(),
                                        )
                                        .await?;
                                        Err(resp.into())
                                    },
                                    _ => Err(resp.into()),
                                }
                            },
                            HttpHandler::new(
                                3,
                                "Sign order item credentials request",
                                &self.client,
                            ),
                        );
                        request_with_retries.await?;
                    }
                    // length of the blinded tokens is less than 1, no
                    // credentials for signing
                }
                CredentialType::SingleUse => {
                    let blinded_creds: Vec<BlindedToken> =
                        match self.client.get_single_use_item_creds(&item.id).await? {
                            Some(item_creds) => {
                                item_creds.creds.iter().map(|t| t.blind()).collect()
                            }
                            None => {
                                let mut csprng = OsRng;
                                let creds: Vec<Token> =
                                    iter::repeat_with(|| Token::random::<Sha512, _>(&mut csprng))
                                        .take(item.quantity as usize)
                                        .collect();

                                let blinded_creds: Vec<BlindedToken> =
                                    creds.iter().map(|t| t.blind()).collect();

                                self.client.init_single_use_item_creds(&item.id, creds).await?;
                                blinded_creds
                            }
                        };

                    let claim_req = ItemCredentialsRequest { blinded_creds };

                    let request_with_retries = FutureRetry::new(
                        || async {
                            let body = serde_json::to_vec(&claim_req)
                                .or(Err(InternalError::SerializationFailed))?;

                            let req = http::Request::builder()
                                .method("PUT")
                                .uri(format!(
                                    "{}/v1/orders/{}/credentials/items/{}/batches/{}",
                                    self.base_url, order_id, item_id, request_id,
                                ))
                                .body(body)?;

                            let resp = self.fetch(req).await?;
                            let app_err: APIError =
                                serde_json::from_slice(resp.body()).unwrap_or(APIError {
                                    code: 0,
                                    message: "unknown".to_string(),
                                    error_code: "".to_string(),
                                    data: Value::Null,
                                });

                            match resp.status() {
                                http::StatusCode::OK => Ok(()),
                                http::StatusCode::CONFLICT => {
                                    Err(InternalError::BadRequest(app_err))
                                }
                                http::StatusCode::NOT_FOUND => Err(InternalError::NotFound),
                                _ => Err(resp.into()),
                            }
                        },
                        HttpHandler::new(3, "Sign order item credentials request", &self.client),
                    );
                    request_with_retries.await?;
                }
                CredentialType::TimeLimited => (), /* Time limited credentials do not require a
                                                    * submission step */
            }
        }
        Ok(request_id)
    }

    #[instrument]
    pub async fn refresh_order_credentials(&self, order_id: &str) -> Result<(), SkusError> {
        if let Some(mut order) = self.client.get_order(order_id).await? {
            if order.is_paid() && order.has_expired(Utc::now().naive_utc()) {
                // if our order has expired, one of two things has happened:
                //   1. our subscription was renewed, resulting in a future expiry and paid
                //      status
                //   2. our subscription was cancelled, resulting in a past expiry and cancelled
                //      status
                // therefore we can be reasonably sure this refresh will only happen once
                order = self.refresh_order(order_id).await?
            }
            if !order.has_expired(Utc::now().naive_utc()) {
                for item in &order.items {
                    if item.credential_type == CredentialType::TimeLimitedV2 {
                        if let Some(credential_expires_at) = self
                            .last_matching_time_limited_v2_credential(&item.id)
                            .await?
                            .map(|cred| cred.valid_to)
                        {
                            if Utc::now().naive_utc() > credential_expires_at {
                                return self.fetch_order_credentials(order_id).await;
                            }
                        }
                    }
                }
            }
            Ok(())
        } else {
            Err(InternalError::NotFound.into())
        }
    }

    #[instrument(err(level = Level::WARN), ret)]
    pub async fn fetch_order_credentials(&self, order_id: &str) -> Result<(), SkusError> {
        let order = match self.client.get_order(order_id).await {
            Ok(Some(order)) => order,
            _ => self.refresh_order(order_id).await?,
        };

        for item in order.items {
            let item_id = &item.id;
            let request_id =
                &self.submit_order_item_credentials_to_sign(order_id, &item.id).await?;

            let request_with_retries = FutureRetry::new(
                || async move {
                    let builder = http::Request::builder().method("GET").uri(format!(
                        "{}/v1/orders/{}/credentials/items/{}/batches/{}",
                        self.base_url, order_id, item_id, request_id
                    ));

                    let req = builder.body(vec![])?;

                    let resp = self.fetch(req).await?;

                    match resp.status() {
                        http::StatusCode::OK => Ok(resp),
                        http::StatusCode::ACCEPTED => {
                            Err(InternalError::RetryLater(delay_from_response(&resp)))
                        }
                        http::StatusCode::NOT_FOUND => Err(InternalError::NotFound),
                        _ => Err(resp.into()),
                    }
                },
                HttpHandler::new(3, "Fetch order credentials request", &self.client),
            );

            let (resp, _) = request_with_retries.await?;

            let resp: Vec<ItemCredentialsResponse> = serde_json::from_slice(resp.body())?;
            event!(Level::DEBUG, "done decoded body");

            let mut time_limited_creds: HashMap<String, Vec<TimeLimitedCredential>> =
                HashMap::new();

            for item_cred in resp {
                match item_cred {
                    ItemCredentialsResponse::TimeLimitedV2 {
                        item_id,
                        blinded_creds,
                        signed_creds,
                        batch_proof,
                        public_key,
                        issuer_id,
                        valid_from,
                        valid_to,
                        ..
                    } => {
                        if let Some(item_creds) =
                            self.client.get_time_limited_v2_creds(&item_id).await?
                        {
                            let from =
                                NaiveDateTime::parse_from_str(&valid_from, "%Y-%m-%dT%H:%M:%SZ")
                                    .map_err(|_| {
                                        InternalError::InvalidResponse(
                                            "Could not parse valid from".to_string(),
                                        )
                                    })?;
                            let to = NaiveDateTime::parse_from_str(&valid_to, "%Y-%m-%dT%H:%M:%SZ")
                                .map_err(|_| {
                                    InternalError::InvalidResponse(
                                        "Could not parse valid to".to_string(),
                                    )
                                })?;

                            if let Some(unblinded_creds) = item_creds.unblinded_creds {
                                if unblinded_creds.iter().any(|stored_cred| {
                                    to == (stored_cred).valid_to && from == (stored_cred).valid_from
                                }) {
                                    continue;
                                }
                            } else {
                                return Err(SkusError(InternalError::ItemCredentialsMissing));
                            }

                            // Rederive blinded creds so that the proof will fail if different creds
                            // were signed
                            let mut my_blinded_creds: HashMap<String, Token> = item_creds
                                .creds
                                .into_iter()
                                .map(|t| (t.blind().encode_base64(), t))
                                .collect();

                            // okay, right here we need to filter out all of the
                            // creds that do not have a blinded token that match
                            // prior to the batch proof check.
                            let mut bucket_blinded_creds: Vec<BlindedToken> = Vec::new();
                            let mut bucket_creds: Vec<Token> = Vec::new();

                            for sbc in blinded_creds {
                                if let Some(t) = my_blinded_creds.remove(&sbc.encode_base64()) {
                                    bucket_blinded_creds.push(sbc);
                                    bucket_creds.push(t);
                                }
                            }

                            // perform the batch proof of the bucket of blinded/signed creds
                            // note this verify and unblind does not verify the bucket_creds
                            // are the unblinded form of the bucket_blinded_creds
                            let unblinded_creds = batch_proof
                                .verify_and_unblind::<Sha512, _>(
                                    &bucket_creds,         // just the creds server says it signed
                                    &bucket_blinded_creds, // the blinded creds
                                    &signed_creds,         // the signed creds from server
                                    &public_key,           // the server's public key
                                )
                                .or(Err(InternalError::InvalidProof))?;

                            // append the time limited v2 item credential to the store
                            self.client
                                .append_time_limited_v2_item_unblinded_creds(
                                    &item_id,
                                    &issuer_id,
                                    unblinded_creds,
                                    &valid_from,
                                    &valid_to,
                                )
                                .await?;
                        }
                    }
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
                            // Rederive blinded creds so that the proof will fail if different creds
                            // were signed
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
                                .complete_single_use_item_creds(
                                    &item_id,
                                    &issuer_id,
                                    unblinded_creds,
                                )
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
                                .and_hms_opt(0, 0, 0)
                                .unwrap(), //  guaranteed to succeed because of (0, 0, 0)
                            expires_at: NaiveDate::parse_from_str(&expires_at, "%Y-%m-%d")
                                .map_err(|_| {
                                    InternalError::InvalidResponse(
                                        "Could not parse expires at".to_string(),
                                    )
                                })?
                                .and_hms_opt(0, 0, 0)
                                .unwrap(), //  guaranteed to succeed because of (0, 0, 0)
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
                self.client.store_time_limited_creds(&item_id, item_creds).await?;
            }
        }

        Ok(())
    }
}
