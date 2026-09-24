/*!
* A module implementing a *terminal user interface* capable of visualizing all information stored in
* [progress trees](../tree/struct.Root.html).
*
* **Please note** that it is behind the `render-tui` feature toggle, which is enabled by default.
*
* # Example
*
* ```should_panic
* # fn main() -> Result<(), Box<dyn std::error::Error>> {
* use futures::task::{LocalSpawnExt, SpawnExt};
* use prodash::render::tui::ticker;
* use prodash::Root;
* // obtain a progress tree
* let root = prodash::tree::Root::new();
* // Configure the gui, provide it with a handle to the ever-changing tree
* let render_fut = prodash::render::tui::render(
*     std::io::stdout(),
*     root.downgrade(),
*     prodash::render::tui::Options {
*         title: "minimal example".into(),
*         ..Default::default()
*     }
* )?;
* // As it runs forever, we want a way to stop it.
* let (render_fut, abort_handle) = futures::future::abortable(render_fut);
* let pool = futures::executor::LocalPool::new();
* // Spawn the gui into the background…
* let gui = pool.spawner().spawn_with_handle(async { render_fut.await.ok(); () })?;
* // …and run tasks which provide progress
* pool.spawner().spawn_local({
*     use futures::StreamExt;
*     let mut progress = root.add_child("task");
*     async move {
*         progress.init(None, None);
*         let mut count = 0;
*         let  mut ticks = ticker(std::time::Duration::from_millis(100));
*         while let Some(_) = ticks.next().await {
*             progress.set(count);
*             count += 1;
*         }
*     }
* })?;
* // …when we are done, tell the GUI to stop
* abort_handle.abort();
* //…and wait until it is done
* futures::executor::block_on(gui);
* # Ok(())
* # }
* ```
*/
mod draw;
mod engine;
mod utils;

pub use engine::*;
/// Useful for bringing up the TUI without bringing in the `tui` crate yourself
pub use tui as tui_export;
pub use utils::ticker;
