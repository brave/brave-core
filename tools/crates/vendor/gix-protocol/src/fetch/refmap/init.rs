use std::collections::HashSet;

use bstr::{BString, ByteVec};
use gix_features::progress::Progress;

use crate::{
    fetch,
    fetch::{
        refmap::{Mapping, Source, SpecIndex},
        RefMap,
    },
    transport::client::Transport,
};

/// The error returned by [`RefMap::new()`].
#[derive(Debug, thiserror::Error)]
#[allow(missing_docs)]
pub enum Error {
    #[error("The object format {format:?} as used by the remote is unsupported")]
    UnknownObjectFormat { format: BString },
    #[error(transparent)]
    MappingValidation(#[from] gix_refspec::match_group::validate::Error),
    #[error(transparent)]
    ListRefs(#[from] crate::ls_refs::Error),
}

/// For use in [`RefMap::new()`].
#[derive(Debug, Clone)]
pub struct Options {
    /// Use a two-component prefix derived from the ref-spec's source, like `refs/heads/`  to let the server pre-filter refs
    /// with great potential for savings in traffic and local CPU time. Defaults to `true`.
    pub prefix_from_spec_as_filter_on_remote: bool,
    /// A list of refspecs to use as implicit refspecs which won't be saved or otherwise be part of the remote in question.
    ///
    /// This is useful for handling `remote.<name>.tagOpt` for example.
    pub extra_refspecs: Vec<gix_refspec::RefSpec>,
}

impl Default for Options {
    fn default() -> Self {
        Options {
            prefix_from_spec_as_filter_on_remote: true,
            extra_refspecs: Vec::new(),
        }
    }
}

impl RefMap {
    /// Create a new instance by obtaining all references on the remote that have been filtered through our remote's
    /// for _fetching_.
    ///
    /// A [context](fetch::Context) is provided to bundle what would be additional parameters,
    /// and [options](Options) are used to further configure the call.
    ///
    /// * `progress` is used if `ls-refs` is invoked on the remote. Always the case when V2 is used.
    /// * `fetch_refspecs` are all explicit refspecs to identify references on the remote that you are interested in.
    ///    Note that these are copied to [`RefMap::refspecs`] for convenience, as `RefMap::mappings` refer to them by index.
    #[allow(clippy::result_large_err)]
    #[maybe_async::maybe_async]
    pub async fn new<T>(
        mut progress: impl Progress,
        fetch_refspecs: &[gix_refspec::RefSpec],
        fetch::Context {
            handshake,
            transport,
            user_agent,
            trace_packetlines,
        }: fetch::Context<'_, T>,
        Options {
            prefix_from_spec_as_filter_on_remote,
            extra_refspecs,
        }: Options,
    ) -> Result<Self, Error>
    where
        T: Transport,
    {
        let _span = gix_trace::coarse!("gix_protocol::fetch::RefMap::new()");
        let null = gix_hash::ObjectId::null(gix_hash::Kind::Sha1); // OK to hardcode Sha1, it's not supposed to match, ever.

        let all_refspecs = {
            let mut s: Vec<_> = fetch_refspecs.to_vec();
            s.extend(extra_refspecs.clone());
            s
        };
        let remote_refs = match handshake.refs.take() {
            Some(refs) => refs,
            None => {
                crate::ls_refs(
                    transport,
                    &handshake.capabilities,
                    |_capabilities, arguments, features| {
                        features.push(user_agent);
                        if prefix_from_spec_as_filter_on_remote {
                            let mut seen = HashSet::new();
                            for spec in &all_refspecs {
                                let spec = spec.to_ref();
                                if seen.insert(spec.instruction()) {
                                    let mut prefixes = Vec::with_capacity(1);
                                    spec.expand_prefixes(&mut prefixes);
                                    for mut prefix in prefixes {
                                        prefix.insert_str(0, "ref-prefix ");
                                        arguments.push(prefix);
                                    }
                                }
                            }
                        }
                        Ok(crate::ls_refs::Action::Continue)
                    },
                    &mut progress,
                    trace_packetlines,
                )
                .await?
            }
        };
        let num_explicit_specs = fetch_refspecs.len();
        let group = gix_refspec::MatchGroup::from_fetch_specs(all_refspecs.iter().map(gix_refspec::RefSpec::to_ref));
        let (res, fixes) = group
            .match_lhs(remote_refs.iter().map(|r| {
                let (full_ref_name, target, object) = r.unpack();
                gix_refspec::match_group::Item {
                    full_ref_name,
                    target: target.unwrap_or(&null),
                    object,
                }
            }))
            .validated()?;

        let mappings = res.mappings;
        let mappings = mappings
            .into_iter()
            .map(|m| Mapping {
                remote: m.item_index.map_or_else(
                    || {
                        Source::ObjectId(match m.lhs {
                            gix_refspec::match_group::SourceRef::ObjectId(id) => id,
                            _ => unreachable!("no item index implies having an object id"),
                        })
                    },
                    |idx| Source::Ref(remote_refs[idx].clone()),
                ),
                local: m.rhs.map(std::borrow::Cow::into_owned),
                spec_index: if m.spec_index < num_explicit_specs {
                    SpecIndex::ExplicitInRemote(m.spec_index)
                } else {
                    SpecIndex::Implicit(m.spec_index - num_explicit_specs)
                },
            })
            .collect();

        let object_hash = extract_object_format(handshake)?;
        Ok(RefMap {
            mappings,
            refspecs: fetch_refspecs.to_vec(),
            extra_refspecs,
            fixes,
            remote_refs,
            object_hash,
        })
    }
}

/// Assume sha1 if server says nothing, otherwise configure anything beyond sha1 in the local repo configuration
#[allow(clippy::result_large_err)]
fn extract_object_format(outcome: &crate::handshake::Outcome) -> Result<gix_hash::Kind, Error> {
    use bstr::ByteSlice;
    let object_hash =
        if let Some(object_format) = outcome.capabilities.capability("object-format").and_then(|c| c.value()) {
            let object_format = object_format.to_str().map_err(|_| Error::UnknownObjectFormat {
                format: object_format.into(),
            })?;
            match object_format {
                "sha1" => gix_hash::Kind::Sha1,
                unknown => return Err(Error::UnknownObjectFormat { format: unknown.into() }),
            }
        } else {
            gix_hash::Kind::Sha1
        };
    Ok(object_hash)
}
