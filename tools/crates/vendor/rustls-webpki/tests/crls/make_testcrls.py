import argparse
from pathlib import Path

from cryptography import x509
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.serialization import Encoding
from cryptography.hazmat.backends import default_backend
from cryptography.x509.oid import NameOID
import datetime

NOT_BEFORE: datetime.datetime = datetime.datetime.utcfromtimestamp(0x1FEDF00D - 30)
NOT_AFTER: datetime.datetime = datetime.datetime.utcfromtimestamp(0x1FEDF00D + 30)

PRIVATE_KEY: ec.EllipticCurvePrivateKey = ec.generate_private_key(
    ec.SECP256R1(), default_backend()
)

EG_URI: x509.UniformResourceIdentifier = x509.UniformResourceIdentifier(
    "http://example.com/crl.1.der"
)


def write_der(path: str, content: bytes, force: bool) -> None:
    # Avoid churn from regenerating existing on-disk resources unless force is enabled.
    out_path = Path(path)
    if out_path.exists() and not force:
        return None

    with out_path.open("wb") as f:
        f.write(content)


def subject_name(test_name: str) -> x509.Name:
    return x509.Name(
        [
            x509.NameAttribute(
                NameOID.COMMON_NAME, "crl-distrib-point-test.example.com"
            ),
            x509.NameAttribute(NameOID.ORGANIZATION_NAME, test_name),
        ]
    )


def generate_crl(
    name: str,
    issuing_distribution_point: x509.IssuingDistributionPoint,
) -> x509.CertificateRevocationList:
    crl_builder: x509.CertificateRevocationListBuilder = (
        x509.CertificateRevocationListBuilder()
    )
    crl_builder = crl_builder.issuer_name(subject_name(name))
    crl_builder = crl_builder.last_update(NOT_BEFORE)
    crl_builder = crl_builder.next_update(NOT_AFTER)
    crl_builder = crl_builder.add_extension(
        x509.CRLNumber(x509.random_serial_number()), critical=False
    )
    crl_builder = crl_builder.add_extension(issuing_distribution_point, critical=True)

    return crl_builder.sign(
        private_key=PRIVATE_KEY,
        algorithm=hashes.SHA256(),
    )


def only_user_certs() -> None:
    name = "only_user_certs"
    write_der(
        f"crl.idp.{name}.der",
        generate_crl(
            name,
            x509.IssuingDistributionPoint(
                only_contains_user_certs=True,
                only_contains_ca_certs=False,
                indirect_crl=False,
                only_contains_attribute_certs=False,
                only_some_reasons=None,
                full_name=[EG_URI],
                relative_name=None,
            ),
        ).public_bytes(Encoding.DER),
        args.force,
    )


def only_ca_certs() -> None:
    name = "only_ca_certs"
    write_der(
        f"crl.idp.{name}.der",
        generate_crl(
            name,
            x509.IssuingDistributionPoint(
                only_contains_user_certs=False,
                only_contains_ca_certs=True,
                indirect_crl=False,
                only_contains_attribute_certs=False,
                only_some_reasons=None,
                full_name=[EG_URI],
                relative_name=None,
            ),
        ).public_bytes(Encoding.DER),
        args.force,
    )


def indirect_crl() -> None:
    name = "indirect_crl"
    write_der(
        f"crl.idp.{name}.der",
        generate_crl(
            name,
            x509.IssuingDistributionPoint(
                only_contains_user_certs=False,
                only_contains_ca_certs=False,
                indirect_crl=True,
                only_contains_attribute_certs=False,
                only_some_reasons=None,
                full_name=[EG_URI],
                relative_name=None,
            ),
        ).public_bytes(Encoding.DER),
        args.force,
    )


def only_attribute_certs() -> None:
    name = "only_attribute_certs"
    write_der(
        f"crl.idp.{name}.der",
        generate_crl(
            name,
            x509.IssuingDistributionPoint(
                only_contains_user_certs=False,
                only_contains_ca_certs=False,
                indirect_crl=False,
                only_contains_attribute_certs=True,
                only_some_reasons=None,
                full_name=[EG_URI],
                relative_name=None,
            ),
        ).public_bytes(Encoding.DER),
        args.force,
    )


def only_some_reasons() -> None:
    name = "only_some_reasons"
    write_der(
        f"crl.idp.{name}.der",
        generate_crl(
            name,
            x509.IssuingDistributionPoint(
                only_contains_user_certs=False,
                only_contains_ca_certs=False,
                indirect_crl=False,
                only_contains_attribute_certs=False,
                only_some_reasons=frozenset(
                    [
                        x509.ReasonFlags.key_compromise,
                        x509.ReasonFlags.affiliation_changed,
                    ]
                ),
                full_name=[EG_URI],
                relative_name=None,
            ),
        ).public_bytes(Encoding.DER),
        args.force,
    )


def name_relative_to_issuer() -> None:
    name = "name_relative_to_issuer"
    write_der(
        f"crl.idp.{name}.der",
        generate_crl(
            name,
            x509.IssuingDistributionPoint(
                only_contains_user_certs=False,
                only_contains_ca_certs=False,
                indirect_crl=False,
                only_contains_attribute_certs=False,
                only_some_reasons=None,
                full_name=None,
                relative_name=x509.RelativeDistinguishedName(
                    [x509.NameAttribute(NameOID.COMMON_NAME, "example.com")]
                ),
            ),
        ).public_bytes(Encoding.DER),
        args.force,
    )


def no_distribution_point_name() -> None:
    name = "no_distribution_point_name"
    write_der(
        f"crl.idp.{name}.der",
        generate_crl(
            name,
            x509.IssuingDistributionPoint(
                only_contains_user_certs=True,
                only_contains_ca_certs=False,
                indirect_crl=False,
                only_contains_attribute_certs=False,
                only_some_reasons=None,
                full_name=None,
                relative_name=None,
            ),
        ).public_bytes(Encoding.DER),
        args.force,
    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--with-only-user-certs",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Generate CRL with only user certificates.",
    )
    parser.add_argument(
        "--with-only-ca-certs",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Generate CRL with only CA certificates.",
    )
    parser.add_argument(
        "--with-indirect-crl",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Generate CRL marked as an indirect CRL",
    )
    parser.add_argument(
        "--with-only-attribute-certs",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Generate CRL with only attribute certificates.",
    )
    parser.add_argument(
        "--with-only-some-reasons",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Generate CRL with only some revocation reasons in-scope.",
    )
    parser.add_argument(
        "--with-name-relative-to-issuer",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Generate CRL with IDP extension with distribution point name relative to issuer",
    )
    parser.add_argument(
        "--with-no-distribution-point-name",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Generate CRL with IDP extension withno distribution point name",
    )
    parser.add_argument(
        "--force",
        action=argparse.BooleanOptionalAction,
        default=False,
        help="Overwrite existing certs",
    )
    args = parser.parse_args()

    if args.with_only_user_certs:
        only_user_certs()
    if args.with_only_ca_certs:
        only_ca_certs()
    if args.with_indirect_crl:
        indirect_crl()
    if args.with_only_attribute_certs:
        only_attribute_certs()
    if args.with_only_some_reasons:
        only_some_reasons()
    if args.with_name_relative_to_issuer:
        name_relative_to_issuer()
    if args.with_no_distribution_point_name:
        no_distribution_point_name()
