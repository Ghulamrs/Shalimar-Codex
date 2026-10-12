# rev49 P0: named AST node IDs checkpoint

Replaced the custom template `AstId<T>` with 20 concrete, named node ID types, each holding a pool number, index, and arena identity. Checked `AstArena::node()` access verifies the owning arena. `AstRef<T>` and `astCast<T>` remain as migration bridges and must still be removed before P0 completion. Standard C++98 STL templates remain permitted.
