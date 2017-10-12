
World is composed of voxels broken into chunks along the x & z axis.

Voxel size is 1m^3.

Chunk-size TBD 32x32. Trade off between mesh sizes (draw calls vs culling), generation / serialisation time. Constrains propagation from a source (light / liquid) to terminate naturally in less than chunk-size steps.

During play there is an N x N square of chunks around the player which are in memory. A smaller square of (N - 1) x (N - 1) chunks are rendered. Because the outer ring of chunks are not rendered things like light and liquid propagation from unloaded chunks into the outer ring are unimportant.

