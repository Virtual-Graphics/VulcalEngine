# Inspector Asset Import Design

Date: 2026-11-05

## Goal

Give the Mesh Renderer inspector a contextual way to import and assign mesh and material source files. Centralize source-type dispatch in `apps/asset_import`, keep `.obj` and `.vmat` as editable project sources, and keep `.vmesh` and `.vmatc` as generated runtime files.

## Design Decisions

`apps::asset_import::AssetImporter` is the public import entry point. Its `import(source_file, assets_directory, library_directory)` operation dispatches `.obj` and `.vmat` sources to their existing type-specific baking implementations and returns the source asset's `AssetID`. Extension matching is case-insensitive. Unsupported extensions fail with an error. The editor uses this facade instead of calling the specialized importers directly.

The specialized importers remain responsible for format parsing and writing the cooked asset. They continue to create or read the source sidecar metadata, preserve the asset ID on reimport, and verify that the metadata type matches the source type. The facade does not own editor UI, scene components, file dialogs, or runtime caches.

Source files must resolve to regular files under the current project's `Assets` directory. The file dialog opens at that directory and filters to `.obj` for mesh assignment or `.vmat` for material assignment. The importer remains the authority for path validation, so selecting an external file is rejected and never copied into the project.

## Editor Interaction

Remove the OBJ and Material import commands from the File menu. In the selected entity's Mesh Renderer inspector, show an ellipsis button after each asset UUID. The mesh button opens the OBJ picker; the material button opens the VMAT picker. Disable both buttons when no project is open.

Canceling a picker is a no-op. After a successful import, assign the returned ID to the corresponding field on the entity whose inspector row initiated the operation, then invalidate that asset in both `AssetManager` and `RenderAssetCache`. Reimporting the same source preserves its metadata ID and refreshes the cached cooked data.

The inspector receives callbacks from `EditorLayer` and does not depend directly on `EditorActions`. `EditorLayer` wraps each callback in its existing `run_operation` handler. `EditorActions` owns the dialog workflow, invokes `AssetImporter`, assigns the ID to the target entity's existing Mesh Renderer component, and invalidates caches. Import and dialog failures continue to surface through the existing Operation Failed dialog; a failed import leaves the component's previous ID unchanged.

## Alternatives Considered

- Let the inspector call `EditorActions` directly. This couples a panel to a concrete workflow service and bypasses the layer's common error wrapper unless the error handling is duplicated.
- Keep import commands in the File menu and assign to the current selection. This keeps the context implicit and makes it easier to import the wrong asset type for the field being edited.
- Add a pluggable importer registry. Only OBJ and VMAT are supported now, so extension dispatch in one facade is sufficient without adding registration and lifecycle machinery.

The callback boundary through `EditorLayer` is selected because it keeps the panel independent while preserving the current error presentation. Contextual field buttons are selected because they make the target field and source type explicit.

## Data Flow

1. The inspector button invokes the mesh or material callback for its entity.
2. `EditorLayer` runs the callback through `run_operation`.
3. `EditorActions` opens the corresponding filtered picker, rooted at the project's Assets directory.
4. If a file was selected, `AssetImporter` dispatches by extension to the matching baker. The baker validates the path and source, preserves or creates metadata, and writes `.vmesh` or `.vmatc` under the library directory.
5. `EditorActions` assigns the returned ID to the requested Mesh Renderer field and invalidates both asset caches for that ID.
6. Cancellation exits without mutation. Any exception is presented by `run_operation` and leaves the field's existing ID unchanged.

## Verification

- Test that `AssetImporter` dispatches OBJ and VMAT sources to the correct baked formats, and rejects unsupported extensions and sources outside `Assets`.
- Test that mesh and material actions assign only their corresponding field and that reimport preserves IDs while refreshing loaded CPU/GPU cache entries.
- Test that cancellation and import failure preserve the current field ID and cached asset state.
- Run the focused asset-import and editor-action tests, then the repository's normal build and test suite.

## Scope Exclusions

This change does not add an `Asset` inheritance hierarchy, change `AssetManager`'s loading model, copy external files into `Assets`, add drag-and-drop, or add file-watcher-driven reimport. Runtime loading continues to use only the cooked formats.
