// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use core::convert::{TryFrom, TryInto};

use chrono::{DateTime, Utc};
use futures_retry::FutureRetry;
use serde::{Deserialize, Serialize};
use std::str;
use tracing::{event, instrument, Level};

#[cfg(feature = "e2e_test")]
use serde_json::{json, to_vec};

use crate::errors::{InternalError, SkusError};
use crate::http::HttpHandler;
use crate::models::*;
use crate::sdk::SDK;
use crate::{HTTPClient, StorageClient};

#[derive(Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
struct OrderMetadataResponse {
    stripe_checkout_session_id: Option<String>,
    payment_processor: Option<String>,
    num_intervals: Option<usize>,
    num_per_interval: Option<usize>,
}

impl From<OrderMetadataResponse> for OrderMetadata {
    fn from(order_metadata: OrderMetadataResponse) -> Self {
        OrderMetadata {
            stripe_checkout_session_id: order_metadata.stripe_checkout_session_id,
            payment_processor: order_metadata.payment_processor,
            num_intervals: order_metadata.num_intervals,
            num_per_interval: order_metadata.num_per_interval,
        }
    }
}

#[derive(Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
struct OrderItemResponse {
    id: String,
    order_id: String,
    sku: String,
    created_at: DateTime<Utc>,
    updated_at: DateTime<Utc>,
    currency: String,
    quantity: i32,
    price: String,
    subtotal: String,
    location: String,
    description: String,
    credential_type: CredentialType,
    // TODO add credential_version
    // TODO add accepted payment types
}

impl TryFrom<OrderItemResponse> for OrderItem {
    type Error = SkusError;

    fn try_from(order_item: OrderItemResponse) -> Result<Self, Self::Error> {
        let price = order_item.price.parse::<f64>().map_err(|_| {
            InternalError::InvalidResponse("Could not parse unit price".to_string())
        })?;
        let subtotal = order_item
            .subtotal
            .parse::<f64>()
            .map_err(|_| InternalError::InvalidResponse("Could not parse subtotal".to_string()))?;
        Ok(OrderItem {
            id: order_item.id,
            order_id: order_item.order_id,
            sku: order_item.sku,
            created_at: order_item.created_at.naive_utc(),
            updated_at: order_item.updated_at.naive_utc(),
            currency: order_item.currency,
            quantity: order_item.quantity,
            price,
            subtotal,
            location: order_item.location,
            description: order_item.description,
            credential_type: order_item.credential_type,
            //valid_for: order_item.valid_for,
        })
    }
}

#[derive(Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
struct OrderResponse {
    id: String,
    created_at: DateTime<Utc>,
    currency: String,
    updated_at: DateTime<Utc>,
    total_price: String,
    location: String,
    merchant_id: String,
    status: String,
    items: Vec<OrderItemResponse>,
    metadata: Option<OrderMetadataResponse>,
    expires_at: Option<DateTime<Utc>>,
    last_paid_at: Option<DateTime<Utc>>,
}

impl TryFrom<OrderResponse> for Order {
    type Error = SkusError;

    fn try_from(order: OrderResponse) -> Result<Self, Self::Error> {
        let total_price = order.total_price.parse::<f64>().map_err(|_| {
            InternalError::InvalidResponse("Could not parse total price".to_string())
        })?;
        let items: Result<Vec<OrderItem>, _> =
            order.items.into_iter().map(|item| item.try_into()).collect();
        Ok(Order {
            id: order.id,
            created_at: order.created_at.naive_utc(),
            currency: order.currency,
            updated_at: order.updated_at.naive_utc(),
            total_price,
            location: order.location,
            merchant_id: order.merchant_id,
            status: order.status,
            items: items?,
            metadata: order.metadata.map(|m| m.into()),
            expires_at: order.expires_at.map(|ex| ex.naive_utc()),
            last_paid_at: order.last_paid_at.map(|lp| lp.naive_utc()),
            //valid_for: order.valid_for,
        })
    }
}

impl<U> SDK<U>
where
    U: HTTPClient + StorageClient,
{
    #[cfg(feature = "e2e_test")]
    pub async fn remove_last_n_creds(&self, order_id: &str, n: usize) -> Result<(), SkusError> {
        let order = self.client.get_order(order_id).await?;
        // iterate over all order items
        for item in order.unwrap().items {
            self.client.delete_n_item_creds(&item.id, n).await?;
        }
        Ok(())
    }
    #[cfg(feature = "e2e_test")]
    pub async fn create_order(&self, kind: &str) -> Result<Order, SkusError> {
        let sku = match kind {
            "trial" => {
                "AgEVc2VhcmNoLmJyYXZlLnNvZnR3YXJlAh9zZWFyY2ggY2xvc2VkIGJldGEgcHJvZ3JhbSBkZW1vAAIWc2t1PXNlYXJjaC1iZXRhLWFjY2VzcwACB3ByaWNlPTAAAgxjdXJyZW5jeT1CQVQAAi1kZXNjcmlwdGlvbj1TZWFyY2ggY2xvc2VkIGJldGEgcHJvZ3JhbSBhY2Nlc3MAAhpjcmVkZW50aWFsX3R5cGU9c2luZ2xlLXVzZQAABiB3uXfAAkNSRQd24jSauRny3VM0BYZ8yOclPTEgPa0xrA=="
            }
            "paid" => {
                "MDAyOWxvY2F0aW9uIHRvZ2V0aGVyLmJzZy5icmF2ZS5zb2Z0d2FyZQowMDMwaWRlbnRpZmllciBicmF2ZS10b2dldGhlci1wYWlkIHNrdSB0b2tlbiB2MQowMDIwY2lkIHNrdT1icmF2ZS10b2dldGhlci1wYWlkCjAwMTBjaWQgcHJpY2U9NQowMDE1Y2lkIGN1cnJlbmN5PVVTRAowMDQzY2lkIGRlc2NyaXB0aW9uPU9uZSBtb250aCBwYWlkIHN1YnNjcmlwdGlvbiBmb3IgQnJhdmUgVG9nZXRoZXIKMDAyNWNpZCBjcmVkZW50aWFsX3R5cGU9dGltZS1saW1pdGVkCjAwMjZjaWQgY3JlZGVudGlhbF92YWxpZF9kdXJhdGlvbj1QMU0KMDAyZnNpZ25hdHVyZSDKLJ7NuuzP3KdmTdVnn0dI3JmIfNblQKmY+WBJOqnQJAo="
            }
            "beta" => {
                "AgEVc2VhcmNoLmJyYXZlLnNvZnR3YXJlAh9zZWFyY2ggY2xvc2VkIGJldGEgcHJvZ3JhbSBkZW1vAAIWc2t1PXNlYXJjaC1iZXRhLWFjY2VzcwACB3ByaWNlPTAAAgxjdXJyZW5jeT1CQVQAAi1kZXNjcmlwdGlvbj1TZWFyY2ggY2xvc2VkIGJldGEgcHJvZ3JhbSBhY2Nlc3MAAhpjcmVkZW50aWFsX3R5cGU9c2luZ2xlLXVzZQAABiB3uXfAAkNSRQd24jSauRny3VM0BYZ8yOclPTEgPa0xrA=="
            }
            "tlv2_e2e" => {
                "MDAzMWxvY2F0aW9uIGZyZWUudGltZS5saW1pdGVkLnYyLmJyYXZlLnNvZnR3YXJlCjAwMjhpZGVudGlmaWVyIGZyZWUtdGltZS1saW1pdGVkLXYyLWRldgowMDI1Y2lkIHNrdT1mcmVlLXRpbWUtbGltaXRlZC12Mi1kZXYKMDAxMGNpZCBwcmljZT0wCjAwMTVjaWQgY3VycmVuY3k9VVNECjAwMmRjaWQgZGVzY3JpcHRpb249ZnJlZS10aW1lLWxpbWl0ZWQtdjItZGV2CjAwMjhjaWQgY3JlZGVudGlhbF90eXBlPXRpbWUtbGltaXRlZC12MgowMDI2Y2lkIGNyZWRlbnRpYWxfdmFsaWRfZHVyYXRpb249UDFNCjAwMWZjaWQgaXNzdWVyX3Rva2VuX2J1ZmZlcj0zMAowMDFmY2lkIGlzc3Vlcl90b2tlbl9vdmVybGFwPTEKMDAyN2NpZCBhbGxvd2VkX3BheW1lbnRfbWV0aG9kcz1zdHJpcGUKMDAyZnNpZ25hdHVyZSAqgung8GCnS0TDch62es768kupFxaEMD1yMSgJX2apdgo="
            }
            "tlv2_e2e_1m" => {
                "MDAzMGxvY2F0aW9uIHByZW1pdW1mcmVldHJpYWwuYnJhdmVzb2Z0d2FyZS5jb20KMDAyZmlkZW50aWZpZXIgYnJhdmUtZnJlZS0xbS10bHYyIHNrdSB0b2tlbiB2MQowMDFmY2lkIHNrdT1icmF2ZS1mcmVlLTFtLXRsdjIKMDAxMGNpZCBwcmljZT0wCjAwMTVjaWQgY3VycmVuY3k9VVNECjAwNDBjaWQgZGVzY3JpcHRpb249RnJlZSB0cmlhbCBhY2Nlc3MgdG8gQnJhdmUgcHJlbWl1bSBwcm9kdWN0cwowMDI4Y2lkIGNyZWRlbnRpYWxfdHlwZT10aW1lLWxpbWl0ZWQtdjIKMDAyOGNpZCBjcmVkZW50aWFsX3ZhbGlkX2R1cmF0aW9uPVBUNjBTCjAwMWZjaWQgaXNzdWVyX3Rva2VuX2J1ZmZlcj0zMAowMDFmY2lkIGlzc3Vlcl90b2tlbl9vdmVybGFwPTEKMDAyZnNpZ25hdHVyZSCCLkg37iCp1uKAYh7MiUQLjILHDWB7tQh1mMXFISCtYgo="
            }
            "tlv2_e2e_5m" => {
                "MDAzMGxvY2F0aW9uIHByZW1pdW1mcmVldHJpYWwuYnJhdmVzb2Z0d2FyZS5jb20KMDAyZmlkZW50aWZpZXIgYnJhdmUtZnJlZS01bS10bHYyIHNrdSB0b2tlbiB2MQowMDFmY2lkIHNrdT1icmF2ZS1mcmVlLTVtLXRsdjIKMDAxMGNpZCBwcmljZT0wCjAwMTVjaWQgY3VycmVuY3k9VVNECjAwNDBjaWQgZGVzY3JpcHRpb249RnJlZSB0cmlhbCBhY2Nlc3MgdG8gQnJhdmUgcHJlbWl1bSBwcm9kdWN0cwowMDI4Y2lkIGNyZWRlbnRpYWxfdHlwZT10aW1lLWxpbWl0ZWQtdjIKMDAyOWNpZCBjcmVkZW50aWFsX3ZhbGlkX2R1cmF0aW9uPVBUMzAwUwowMDFmY2lkIGlzc3Vlcl90b2tlbl9idWZmZXI9MzAKMDAxZmNpZCBpc3N1ZXJfdG9rZW5fb3ZlcmxhcD0xCjAwMmZzaWduYXR1cmUgBkRRgn1Y5SDmnwnsCfYl3JWpfb/OL5LrFqYezBlc3osK"
            }
            _ => "",
        };

        let quantity = match kind {
            "beta" => 10,
            _ => 1,
        };

        let order = json!({
            "items": [
                {
                    "sku": sku,
                    "quantity": quantity
                }
           ]
        });

        let request_with_retries = FutureRetry::new(
            || async {
                let mut builder = http::Request::builder();
                builder.method("POST");
                builder.uri(format!("{}/v1/orders", self.base_url));

                let body = to_vec(&order).or(Err(InternalError::SerializationFailed))?;

                let req = builder.body(body)?;
                let resp = self.fetch(req).await?;

                match resp.status() {
                    http::StatusCode::CREATED => Ok(resp),
                    _ => Err(resp.into()),
                }
            },
            HttpHandler::new(3, "Create order request", &self.client),
        );
        let (resp, _) = request_with_retries.await?;

        let order: OrderResponse = serde_json::from_slice(resp.body())?;
        let order: Order = order.try_into()?;

        self.client.upsert_order(&order).await?;

        Ok(order)
    }

    #[instrument]
    pub async fn fetch_order(&self, order_id: &str) -> Result<Order, SkusError> {
        let request_with_retries = FutureRetry::new(
            || async {
                let builder = http::Request::builder()
                    .method("GET")
                    .uri(format!("{}/v1/orders/{}", self.base_url, order_id));

                let req = builder.body(vec![])?;
                let resp = self.fetch(req).await?;

                match resp.status() {
                    http::StatusCode::OK => Ok(resp),
                    http::StatusCode::NOT_FOUND => Err(InternalError::NotFound),
                    _ => Err(resp.into()),
                }
            },
            HttpHandler::new(3, "Fetch order request", &self.client),
        );
        let (resp, _) = request_with_retries.await?;

        let order: OrderResponse = serde_json::from_slice(resp.body())?;
        let order: Order = order.try_into()?;

        Ok(order)
    }

    #[instrument]
    // submit_receipt allows for order proof of payment
    pub async fn submit_receipt(&self, order_id: &str, receipt: &str) -> Result<(), InternalError> {
        event!(Level::DEBUG, order_id = order_id, "submit_receipt called");
        let request_with_retries = FutureRetry::new(
            || async {
                let builder = http::Request::builder()
                    .method("POST")
                    .uri(format!("{}/v1/orders/{}/submit-receipt", self.base_url, order_id));

                let receipt_bytes = receipt.as_bytes().to_vec();
                let req =
                    builder.body(receipt_bytes)?;

                let resp = self.fetch(req).await?;
                event!(
                    Level::DEBUG,
                    response = str::from_utf8(resp.body()).unwrap_or("<invalid body>"),
                    "submit_receipt called"
                );
                match resp.status() {
                    http::StatusCode::OK => Ok(resp),
                    http::StatusCode::NOT_FOUND => Err(InternalError::NotFound),
                    _ => Err(resp.into()),
                }
            },
            HttpHandler::new(3, "Submit order receipt", &self.client),
        );

        request_with_retries.await?;

        Ok(())
    }

    #[instrument]
    pub async fn refresh_order(&self, order_id: &str) -> Result<Order, SkusError> {
        event!(Level::DEBUG, order_id = order_id, "refresh_order called",);
        let order = self.fetch_order(order_id).await?;
        self.client.upsert_order(&order).await?;
        Ok(order)
    }
}
