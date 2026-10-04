//! This example constructs a Wasm module from scratch with Walrus.
//!
//! The module we are building implements and exports the `factorial` function,
//! and imports a `env.log` function to observe incremental results.
//!
//! You can run the built Wasm module using Node.js (for example) like this:
//!
//! ```js
//! const fs = require("fs");
//!
//! async function main() {
//!   const bytes = fs.readFileSync("target/out.wasm");
//!   const env = { log: val => console.log(`logged ${val}`), };
//!   const { instance } = await WebAssembly.instantiate(
//!     bytes,
//!     {
//!       env: {
//!         log(val) {
//!           console.log(`log saw ${val}`);
//!         }
//!       }
//!     }
//!   );
//!   const result = instance.exports.factorial(5);
//!   console.log(`factorial(5) = ${result}`);
//! }
//!
//! main();
//! ```

use walrus::ir::*;
use walrus::{FunctionBuilder, Module, ModuleConfig, ValType};

fn main() -> walrus::Result<()> {
    // Construct a new Walrus module.
    let config = ModuleConfig::new();
    let mut module = Module::with_config(config);

    // Import the `log` function.
    let log_type = module.types.add(&[ValType::I32], &[]);
    let (log, _) = module.add_import_func("env", "log", log_type);

    // Building this factorial implementation:
    // https://github.com/WebAssembly/testsuite/blob/7816043/fac.wast#L46-L66
    let mut factorial = FunctionBuilder::new(&mut module.types, &[ValType::I32], &[ValType::I32]);

    // Create our parameter and our two locals.
    let n = module.locals.add(ValType::I32);
    let i = module.locals.add(ValType::I32);
    let res = module.locals.add(ValType::I32);

    factorial
        // Enter the function's body.
        .func_body()
        // (local.set $i (local.get $n))
        .local_get(n)
        .local_set(i)
        // (local.set $res (i32.const 1))
        .i32_const(1)
        .local_set(res)
        .block(None, |done| {
            let done_id = done.id();
            done.loop_(None, |loop_| {
                let loop_id = loop_.id();
                loop_
                    // (call $log (local.get $res))
                    .local_get(res)
                    .call(log)
                    // (i32.eq (local.get $i) (i32.const 0))
                    .local_get(i)
                    .i32_const(0)
                    .binop(BinaryOp::I32Eq)
                    .if_else(
                        None,
                        |then| {
                            // (then (br $done))
                            then.br(done_id);
                        },
                        |else_| {
                            else_
                                // (local.set $res (i32.mul (local.get $i) (local.get $res)))
                                .local_get(i)
                                .local_get(res)
                                .binop(BinaryOp::I32Mul)
                                .local_set(res)
                                // (local.set $i (i32.sub (local.get $i) (i32.const 1))))
                                .local_get(i)
                                .i32_const(1)
                                .binop(BinaryOp::I32Sub)
                                .local_set(i);
                        },
                    )
                    .br(loop_id);
            });
        })
        .local_get(res);

    let factorial = factorial.finish(vec![n], &mut module.funcs);

    // Export the `factorial` function.
    module.exports.add("factorial", factorial);

    // Emit the `.wasm` binary to the `target/out.wasm` file.
    module.emit_wasm_file("target/out.wasm")
}
