// Copyright 2015 Brian Smith.
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHORS DISCLAIM ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

use super::dns_name::{self, IdRole};
use super::ip_address;
use crate::der::{self, FromDer};
use crate::error::{DerTypeId, Error};
use crate::verify_cert::{Budget, PathNode};

// https://tools.ietf.org/html/rfc5280#section-4.2.1.10
pub(crate) fn check_name_constraints(
    constraints: Option<&mut untrusted::Reader<'_>>,
    path: &PathNode<'_>,
    budget: &mut Budget,
) -> Result<(), Error> {
    let constraints = match constraints {
        Some(input) => input,
        None => return Ok(()),
    };

    fn parse_subtrees<'b>(
        inner: &mut untrusted::Reader<'b>,
        subtrees_tag: der::Tag,
    ) -> Result<Option<untrusted::Input<'b>>, Error> {
        if !inner.peek(subtrees_tag.into()) {
            return Ok(None);
        }
        der::expect_tag(inner, subtrees_tag).map(Some)
    }

    let permitted_subtrees = parse_subtrees(constraints, der::Tag::ContextSpecificConstructed0)?;
    let excluded_subtrees = parse_subtrees(constraints, der::Tag::ContextSpecificConstructed1)?;

    for path in path.iter() {
        let result = NameIterator::new(Some(path.cert.subject), path.cert.subject_alt_name)
            .find_map(|result| {
                let name = match result {
                    Ok(name) => name,
                    Err(err) => return Some(Err(err)),
                };

                check_presented_id_conforms_to_constraints(
                    name,
                    permitted_subtrees,
                    excluded_subtrees,
                    budget,
                )
            });

        if let Some(Err(err)) = result {
            return Err(err);
        }
    }

    Ok(())
}

fn check_presented_id_conforms_to_constraints(
    name: GeneralName<'_>,
    permitted_subtrees: Option<untrusted::Input<'_>>,
    excluded_subtrees: Option<untrusted::Input<'_>>,
    budget: &mut Budget,
) -> Option<Result<(), Error>> {
    let subtrees = [
        (Subtrees::PermittedSubtrees, permitted_subtrees),
        (Subtrees::ExcludedSubtrees, excluded_subtrees),
    ];

    fn general_subtree<'b>(input: &mut untrusted::Reader<'b>) -> Result<GeneralName<'b>, Error> {
        der::read_all(der::expect_tag(input, der::Tag::Sequence)?)
    }

    for (subtrees, constraints) in subtrees {
        let mut constraints = match constraints {
            Some(constraints) => untrusted::Reader::new(constraints),
            None => continue,
        };

        let mut has_permitted_subtrees_match = false;
        let mut has_permitted_subtrees_mismatch = false;
        while !constraints.at_end() {
            if let Err(e) = budget.consume_name_constraint_comparison() {
                return Some(Err(e));
            }

            // http://tools.ietf.org/html/rfc5280#section-4.2.1.10: "Within this
            // profile, the minimum and maximum fields are not used with any name
            // forms, thus, the minimum MUST be zero, and maximum MUST be absent."
            //
            // Since the default value isn't allowed to be encoded according to the
            // DER encoding rules for DEFAULT, this is equivalent to saying that
            // neither minimum or maximum must be encoded.
            let base = match general_subtree(&mut constraints) {
                Ok(base) => base,
                Err(err) => return Some(Err(err)),
            };

            let matches = match (name, base) {
                (GeneralName::DnsName(name), GeneralName::DnsName(base)) => {
                    dns_name::presented_id_matches_reference_id(name, IdRole::NameConstraint, base)
                }

                (GeneralName::DirectoryName, GeneralName::DirectoryName) => Ok(
                    // Reject any uses of directory name constraints; we don't implement this.
                    //
                    // Rejecting everything technically confirms to RFC5280:
                    //
                    //   "If a name constraints extension that is marked as critical imposes constraints
                    //    on a particular name form, and an instance of that name form appears in the
                    //    subject field or subjectAltName extension of a subsequent certificate, then
                    //    the application MUST either process the constraint or _reject the certificate_."
                    //
                    // TODO: rustls/webpki#19
                    //
                    // Rejection is achieved by not matching any PermittedSubtrees, and matching all
                    // ExcludedSubtrees.
                    match subtrees {
                        Subtrees::PermittedSubtrees => false,
                        Subtrees::ExcludedSubtrees => true,
                    },
                ),

                (GeneralName::IpAddress(name), GeneralName::IpAddress(base)) => {
                    ip_address::presented_id_matches_constraint(name, base)
                }

                // RFC 4280 says "If a name constraints extension that is marked as
                // critical imposes constraints on a particular name form, and an
                // instance of that name form appears in the subject field or
                // subjectAltName extension of a subsequent certificate, then the
                // application MUST either process the constraint or reject the
                // certificate." Later, the CABForum agreed to support non-critical
                // constraints, so it is important to reject the cert without
                // considering whether the name constraint it critical.
                (GeneralName::Unsupported(name_tag), GeneralName::Unsupported(base_tag))
                    if name_tag == base_tag =>
                {
                    Err(Error::NameConstraintViolation)
                }

                _ => {
                    // mismatch between constraint and name types; continue with current
                    // name and next constraint
                    continue;
                }
            };

            match (subtrees, matches) {
                (Subtrees::PermittedSubtrees, Ok(true)) => {
                    has_permitted_subtrees_match = true;
                }

                (Subtrees::PermittedSubtrees, Ok(false)) => {
                    has_permitted_subtrees_mismatch = true;
                }

                (Subtrees::ExcludedSubtrees, Ok(true)) => {
                    return Some(Err(Error::NameConstraintViolation));
                }

                (Subtrees::ExcludedSubtrees, Ok(false)) => (),
                (_, Err(err)) => return Some(Err(err)),
            }
        }

        if has_permitted_subtrees_mismatch && !has_permitted_subtrees_match {
            // If there was any entry of the given type in permittedSubtrees, then
            // it required that at least one of them must match. Since none of them
            // did, we have a failure.
            return Some(Err(Error::NameConstraintViolation));
        }
    }

    None
}

#[derive(Clone, Copy)]
enum Subtrees {
    PermittedSubtrees,
    ExcludedSubtrees,
}

pub(crate) struct NameIterator<'a> {
    subject_alt_name: Option<untrusted::Reader<'a>>,
    subject_directory_name: Option<untrusted::Input<'a>>,
}

impl<'a> NameIterator<'a> {
    pub(crate) fn new(
        subject: Option<untrusted::Input<'a>>,
        subject_alt_name: Option<untrusted::Input<'a>>,
    ) -> Self {
        NameIterator {
            subject_alt_name: subject_alt_name.map(untrusted::Reader::new),

            // If `subject` is present, we always consider it as a `DirectoryName`.
            subject_directory_name: subject,
        }
    }
}

impl<'a> Iterator for NameIterator<'a> {
    type Item = Result<GeneralName<'a>, Error>;

    fn next(&mut self) -> Option<Self::Item> {
        if let Some(subject_alt_name) = &mut self.subject_alt_name {
            // https://bugzilla.mozilla.org/show_bug.cgi?id=1143085: An empty
            // subjectAltName is not legal, but some certificates have an empty
            // subjectAltName. Since we don't support CN-IDs, the certificate
            // will be rejected either way, but checking `at_end` before
            // attempting to parse the first entry allows us to return a better
            // error code.

            if !subject_alt_name.at_end() {
                let err = match GeneralName::from_der(subject_alt_name) {
                    Ok(name) => return Some(Ok(name)),
                    Err(err) => err,
                };

                // Make sure we don't yield any items after this error.
                self.subject_alt_name = None;
                self.subject_directory_name = None;
                return Some(Err(err));
            } else {
                self.subject_alt_name = None;
            }
        }

        if self.subject_directory_name.take().is_some() {
            return Some(Ok(GeneralName::DirectoryName));
        }

        None
    }
}

// It is *not* valid to derive `Eq`, `PartialEq, etc. for this type. In
// particular, for the types of `GeneralName`s that we don't understand, we
// don't even store the value. Also, the meaning of a `GeneralName` in a name
// constraint is different than the meaning of the identically-represented
// `GeneralName` in other contexts.
#[derive(Clone, Copy)]
pub(crate) enum GeneralName<'a> {
    DnsName(untrusted::Input<'a>),
    DirectoryName,
    IpAddress(untrusted::Input<'a>),
    UniformResourceIdentifier(untrusted::Input<'a>),

    // The value is the `tag & ~(der::CONTEXT_SPECIFIC | der::CONSTRUCTED)` so
    // that the name constraint checking matches tags regardless of whether
    // those bits are set.
    Unsupported(u8),
}

impl<'a> FromDer<'a> for GeneralName<'a> {
    fn from_der(reader: &mut untrusted::Reader<'a>) -> Result<Self, Error> {
        use der::{CONSTRUCTED, CONTEXT_SPECIFIC};
        use GeneralName::*;

        #[allow(clippy::identity_op)]
        const OTHER_NAME_TAG: u8 = CONTEXT_SPECIFIC | CONSTRUCTED | 0;
        const RFC822_NAME_TAG: u8 = CONTEXT_SPECIFIC | 1;
        const DNS_NAME_TAG: u8 = CONTEXT_SPECIFIC | 2;
        const X400_ADDRESS_TAG: u8 = CONTEXT_SPECIFIC | CONSTRUCTED | 3;
        const DIRECTORY_NAME_TAG: u8 = CONTEXT_SPECIFIC | CONSTRUCTED | 4;
        const EDI_PARTY_NAME_TAG: u8 = CONTEXT_SPECIFIC | CONSTRUCTED | 5;
        const UNIFORM_RESOURCE_IDENTIFIER_TAG: u8 = CONTEXT_SPECIFIC | 6;
        const IP_ADDRESS_TAG: u8 = CONTEXT_SPECIFIC | 7;
        const REGISTERED_ID_TAG: u8 = CONTEXT_SPECIFIC | 8;

        let (tag, value) = der::read_tag_and_get_value(reader)?;
        Ok(match tag {
            DNS_NAME_TAG => DnsName(value),
            DIRECTORY_NAME_TAG => DirectoryName,
            IP_ADDRESS_TAG => IpAddress(value),
            UNIFORM_RESOURCE_IDENTIFIER_TAG => UniformResourceIdentifier(value),

            OTHER_NAME_TAG | RFC822_NAME_TAG | X400_ADDRESS_TAG | EDI_PARTY_NAME_TAG
            | REGISTERED_ID_TAG => Unsupported(tag & !(CONTEXT_SPECIFIC | CONSTRUCTED)),

            _ => return Err(Error::BadDer),
        })
    }

    const TYPE_ID: DerTypeId = DerTypeId::GeneralName;
}
