/*
 * Copyright (c) 2012-2020 MIRACL UK Ltd.
 *
 * This file is part of MIRACL Core
 * (see https://github.com/miracl/core).
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// comment out if debugging with print macros !!!
#![cfg_attr(not(feature = "std"), no_std)]

#![allow(clippy::many_single_char_names)]
#![allow(clippy::needless_range_loop)]
#![allow(clippy::manual_memcpy)]
#![allow(clippy::new_without_default)]
pub mod arch;
pub mod aes;
pub mod gcm;
pub mod hmac;
pub mod hash256;
pub mod hash384;
pub mod hash512;
pub mod rand;
pub mod share;
pub mod sha3;
pub mod nhs;
pub mod dilithium;
pub mod kyber;
pub mod x509;
pub mod bn254;
