import argparse
from pathlib import Path
from typing import List

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
PUBLIC_KEY: ec.EllipticCurvePublicKey = PRIVATE_KEY.public_key()


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


def generate_cert(
    name: str,
    distribution_points: List[x509.DistributionPoint],
) -> x509.Certificate:
    cert_builder: x509.CertificateBuilder = x509.CertificateBuilder()
    cert_builder = cert_builder.subject_name(subject_name(name))
    cert_builder = cert_builder.issuer_name(subject_name(name))
    cert_builder = cert_builder.not_valid_before(NOT_BEFORE)
    cert_builder = cert_builder.not_valid_after(NOT_AFTER)
    cert_builder = cert_builder.serial_number(x509.random_serial_number())
    cert_builder = cert_builder.public_key(PUBLIC_KEY)
    cert_builder = cert_builder.add_extension(
        x509.CRLDistributionPoints(distribution_points), critical=False
    )

    return cert_builder.sign(
        private_key=PRIVATE_KEY,
        algorithm=hashes.SHA256(),
        backend=default_backend(),
    )


def with_reasons() -> None:
    name = "with_reasons"
    write_der(
        f"{name}.der",
        generate_cert(
            name,
            [
                x509.DistributionPoint(
                    full_name=[
                        x509.UniformResourceIdentifier("http://example.com/crl.der")
                    ],
                    reasons=frozenset(
                        [
                            x509.ReasonFlags.key_compromise,
                            x509.ReasonFlags.affiliation_changed,
                        ]
                    ),
                    relative_name=None,
                    crl_issuer=None,
                ),
            ],
        ).public_bytes(Encoding.DER),
        args.force,
    )


def with_crl_issuer() -> None:
    name = "with_crl_issuer"
    write_der(
        f"{name}.der",
        generate_cert(
            name,
            [
                x509.DistributionPoint(
                    full_name=None,
                    reasons=None,
                    relative_name=None,
                    crl_issuer=[
                        x509.DirectoryName(
                            x509.Name(
                                [
                                    x509.NameAttribute(
                                        NameOID.COMMON_NAME,
                                        "crl-distrib-point-test.example.com",
                                    ),
                                ]
                            )
                        )
                    ],
                ),
            ],
        ).public_bytes(Encoding.DER),
        args.force,
    )


def with_relative_name() -> None:
    name = "dp_name_relative_to_issuer"
    write_der(
        f"{name}.der",
        generate_cert(
            name,
            [
                x509.DistributionPoint(
                    full_name=None,
                    reasons=None,
                    relative_name=x509.RelativeDistinguishedName(
                        [x509.NameAttribute(NameOID.COMMON_NAME, "example.com")]
                    ),
                    crl_issuer=None,
                ),
            ],
        ).public_bytes(Encoding.DER),
        args.force,
    )


def with_multiple() -> None:
    name = "multiple_distribution_points"
    write_der(
        f"{name}.der",
        generate_cert(
            name,
            [
                x509.DistributionPoint(
                    full_name=[
                        x509.UniformResourceIdentifier("http://example.com/crl.1.der")
                    ],
                    reasons=None,
                    relative_name=None,
                    crl_issuer=None,
                ),
                x509.DistributionPoint(
                    full_name=[
                        x509.UniformResourceIdentifier("http://example.com/crl.2.der"),
                        x509.UniformResourceIdentifier("http://example.com/crl.3.der"),
                    ],
                    reasons=None,
                    relative_name=None,
                    crl_issuer=None,
                ),
            ],
        ).public_bytes(Encoding.DER),
        args.force,
    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--with-reasons",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Generate a certificate with CRL distribution point with reason codes.",
    )
    parser.add_argument(
        "--with-crl-issuer",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Generate a certificate with CRL distribution point with cRLIssuer.",
    )
    parser.add_argument(
        "--with-name-relative-to-issuer",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Generate a certificate with CRL distribution point name relative to issuer.",
    )
    parser.add_argument(
        "--multiple-distribution-points",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Generate a certificate with multiple CRL distribution points.",
    )
    parser.add_argument(
        "--force",
        action=argparse.BooleanOptionalAction,
        default=False,
        help="Overwrite existing certs",
    )
    args = parser.parse_args()

    if args.with_reasons:
        with_reasons()
    if args.with_crl_issuer:
        with_crl_issuer()
    if args.with_name_relative_to_issuer:
        with_relative_name()
    if args.multiple_distribution_points:
        with_multiple()
