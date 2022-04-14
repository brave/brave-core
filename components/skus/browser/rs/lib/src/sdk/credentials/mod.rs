mod fetch;
mod present;

use chrono::Utc;
use tracing::{error, instrument};

use crate::errors::{InternalError, SkusError};
use crate::models::*;
use crate::sdk::SDK;
use crate::storage::Credentials;
use crate::{HTTPClient, StorageClient};

impl<U> SDK<U>
where
    U: HTTPClient + StorageClient,
{
    #[instrument]
    pub async fn delete_order_credentials(&self, order_id: &str) -> Result<(), SkusError> {
        let order = self.client.get_order(order_id).await?;

        if let Some(order) = order {
            for item in order.items {
                self.client.delete_item_creds(&item.id).await?;
            }
        }
        Ok(())
    }

    #[instrument]
    pub async fn matching_order_credential_summary(
        &self,
        order_id: &str,
        domain: &str,
    ) -> Result<Option<CredentialSummary>, SkusError> {
        let order = self.client.get_order(order_id).await?;

        if let Some(order) = order {
            let expires_at = order.expires_at;

            if !order.location_matches(&self.environment, domain) {
                return Err(InternalError::OrderLocationMismatch.into());
            }

            // FIXME only returns summary for first item
            if let Some(item) = order.items.iter().next() {
                return Ok(match item.credential_type {
                    CredentialType::SingleUse => {
                        self.client.get_single_use_item_creds(&item.id).await?.and_then(|creds| {
                            let unblinded_creds = creds.unblinded_creds?;

                            let remaining_credential_count: u32 =
                                unblinded_creds.into_iter().filter(|cred| !cred.spent).count()
                                    as u32;

                            let expires_at = None;
                            let active = remaining_credential_count > 0;

                            Some(CredentialSummary {
                                order,
                                remaining_credential_count,
                                expires_at,
                                active,
                            })
                        })
                    }
                    CredentialType::TimeLimited => {
                        let maybe_cred =
                            self.last_matching_time_limited_credential(&item.id).await?;

                        match maybe_cred {
                            Some(Credential::TimeLimited(cred)) => {
                                // attempt to refresh credentials if we're within 5 days of expiry
                                if Utc::now().naive_utc()
                                    > (cred.expires_at - chrono::Duration::days(5))
                                {
                                    if let Err(e) = self.refresh_order_credentials(order_id).await {
                                        error!(error = %e, "failed to refresh order credentials");
                                    }
                                }
                            }
                            _ => {}
                        };

                        let active = matches!(
                            self.matching_time_limited_credential(&item.id).await,
                            Ok(Some(_))
                        );

                        Some(CredentialSummary {
                            order,
                            remaining_credential_count: 1,
                            expires_at,
                            active,
                        })
                    }
                    CredentialType::TimeLimitedV2 => {
                        let active = matches!(
                            self.matching_time_limited_credential(&item.id).await,
                            Ok(Some(_))
                        );
                        let expires_at = None;

                        Some(CredentialSummary {
                            order,
                            remaining_credential_count: 1,
                            expires_at,
                            active,
                        })
                    }
                });
            }
        }
        Ok(None)
    }

    #[instrument]
    pub async fn matching_time_limited_credential(
        &self,
        item_id: &str,
    ) -> Result<Option<Credential>, SkusError> {
        Ok(self.client.get_time_limited_creds(item_id).await?.and_then(|creds| {
            if let Credentials::TimeLimited(cred) = creds {
                cred.creds
                    .into_iter()
                    .find(|cred| {
                        Utc::now().naive_utc() < cred.expires_at
                            && Utc::now().naive_utc() > cred.issued_at
                    })
                    .map(|cred| Credential::TimeLimited(cred))
            } else {
                None
            }
        }))
    }

    #[instrument]
    pub async fn last_matching_time_limited_credential(
        &self,
        item_id: &str,
    ) -> Result<Option<Credential>, SkusError> {
        Ok(self
            .client
            .get_time_limited_creds(item_id)
            .await?
            // .and_then(|creds| creds.creds.into_iter().last()))
            .and_then(|creds| {
                if let Credentials::TimeLimited(cred) = creds {
                    cred.creds.into_iter().last()
                } else {
                    None
                }
            })
            .map(|cred| Credential::TimeLimited(cred)))
    }

    #[instrument]
    pub async fn matching_credential_summary(
        &self,
        domain: &str,
    ) -> Result<Option<CredentialSummary>, SkusError> {
        if let Some(orders) = self.client.get_orders().await? {
            for order in orders {
                if order.location_matches(&self.environment, domain) {
                    if let Some(value) =
                        self.matching_order_credential_summary(&order.id, domain).await?
                    {
                        return Ok(Some(value));
                    }
                }
            }
        }
        Ok(None)
    }
}
