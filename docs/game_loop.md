# Vulkan-Craft Game Loop

Main game loop like this:

```
    generate spawn chunks
    spawn player

    while not quit do
        move player
        generate local chunks
    end
```


## Chunk generation & caching

* Want to have _n_ x _n_ chunks area of interest around the player.
* When the player moves might need to get extra chunks and lose some chunks.

Let _n_ = 3, X denotes player position at start of frame, x denotes player position at end of frame, o denotes loaded chunk, + denotes chunk which needs to be loaded or generated, - denotes chunk which needs to be unloaded, then the diagram depicts a state where the player just crossed into a new chunk:

```
         +++
         oxo
         oXo
         ---
```

How to go about this?

1. Unload chunks which are no longer needed. Chunk data needs to be persisted to disk, then it can be released from memory. Chunk meshes can be cleaned up when the GPU is finished with them.
2. Load or generate chunks which are now required. Chunk meshes need to be built for the renderer.


### Systems & data structures

1. The game simulation. Keeps chunk data in memory for AI & physics.
2. The renderer. Draws chunks.
3. The streaming system. Asynchronously writes chunk data to disk, reads chunks from disk.


#### Game Simulation

Handle player movement (physics).
Handle block placement.
Handle block destruction.

Whenever a chunk moves out of area of interest of the player remove it from memory.

Whenever the game changes a chunk (place or break block) it needs to:
1. Write the change to disk (send a request to the streaming system).
2. Update or rebuild the render mesh.

