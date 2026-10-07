# Uploading matrices to the GPU and layout gotchas

Notes from 2026-10-04 while starting `src/Matrix4D.h`.

## TODO next session

- [ ] Remove the stray `Matrix3D` line at the bottom of the `Matrix4D` struct (it doesn't compile).
- [ ] Add `inline` to the free functions in `Matrix3D.h`: both `operator*` overloads and `MakeRotationY`.
- [ ] Add `data()`, `alignas(16)` and the `static_assert`s to `Matrix4D` (see below).
- [ ] Add an `explicit Matrix4D(const Matrix3D&)` constructor in `Matrix4D.h`.
- [ ] Do the reading list at the bottom.
- [ ] Upload a recognizable rotation and check it isn't transposed in the shader.

## Getting Matrix4D's data to the GPU

`float n[4][4]` is 16 contiguous floats (64 bytes) with no padding, and the struct is trivially copyable and standard-layout. So `&matrix` already points at `n[0][0]`, and you can pass the object straight in:

```cpp
memcpy(mappedPtr, &matrix, sizeof(Matrix4D));
vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Matrix4D), &matrix);
```

Still add an accessor (it reads more clearly) and lock the layout down so it can't change without you noticing:

```cpp
#include <type_traits>

struct alignas(16) Matrix4D
{
private:
    float n[4][4];
public:
    Matrix4D() = default;
    const float* data() const { return &n[0][0]; }
    // ...
};

static_assert(sizeof(Matrix4D) == 64);
static_assert(std::is_trivially_copyable_v<Matrix4D>);
static_assert(std::is_standard_layout_v<Matrix4D>);
```

`alignas(16)` only matters when Matrix4D is a member of a C++ struct that mirrors a shader block, e.g. `struct { float time; Matrix4D mvp; }`. It puts `mvp` at offset 16, where the shader expects it.

**Where efficiency comes from:**

1. **Push constants:** the cheapest option for one or two matrices that change per draw. Vulkan guarantees at least 128 bytes (two mat4s), and no buffer or descriptor is needed.
2. **A persistently mapped uniform/storage buffer, one per frame in flight:** use this for anything bigger. It works the same way as the vertex buffer upload (`pMappedData` + `memcpy` in `BabysFirstGraphicsPipeline.cpp`). Don't go through a staging buffer for data that changes every frame.

**Matrix order:** C++ storage is column-major (`n[col][row]`). Keep Slang on column-major too, so `mul(M, v)` should be correct. If it looks transposed, check the Slang matrix-layout option.

## Why the fear about Matrix3D was justified

Matrix3D is the one case where the C++ layout and the GPU layout differ, and nothing warns you.

- C++ `float n[3][3]`: 36 bytes, columns 12 bytes apart.
- GPU (std140/std430, which uniform buffers, storage buffers and push constants all use): each column is a vec3, which must start on a 16-byte boundary. So the columns are 16 bytes apart and the total is 48 bytes.

```
byte:      0         12        24        36   48
C++:       [c0 x y z][c1 x y z][c2 x y z]
GPU:       [c0 x y z _][c1 x y z _][c2 x y z _]
                     ^ the shader reads column 1 starting here (byte 16)
```

If you memcpy the 36 bytes, column 0 is right and everything after it is shifted. There's no crash or validation error, just a wrong transform. The gap between consecutive columns is the **stride** (Vulkan calls it `MatrixStride`).

**Ways to handle it:**

1. **Only upload 4x4 matrices (recommended).** A mat4 is identical on both sides. Do the 3x3 math on the CPU, then convert to Matrix4D before uploading.
2. **Use a padded struct for uploading:** `struct GpuMat3 { float cols[3][4]; };` (48 bytes), copied in column by column.
3. **Use scalar block layout:** the `scalarBlockLayout` feature (core in Vulkan 1.2) plus Slang's `-fvk-use-scalar-layout`. vec3s get no padding, but this changes the layout for every buffer in those shaders.

Slang's row-major/column-major setting changes *which* vectors get padded (columns or rows). Keep it on column-major to match the C++ side.

## Why "multiple definition" linker errors happen

- `#include` pastes the header's text into the `.cpp`. Each `.cpp` compiles on its own into a `.o`, and the linker combines them afterwards.
- A non-`inline` function defined in a header gets compiled into **every** `.o` that includes it. The linker then sees two `operator*` and fails.
- Include guards don't help. They only stop the header being pasted twice into the *same* `.cpp`.
- `inline` tells the linker that duplicate copies are expected and identical, so it keeps one. That's why `Vector3D.h` is fine.
- Methods defined inside a struct body are implicitly `inline`. Only free functions need the keyword.
- `Matrix3D.h` builds today only because just one `.cpp` includes it so far. That changes once `Matrix4D.h` is used in more than one place.

## Where the mat3 → mat4 conversion goes

Put it in `Matrix4D.h` as `explicit Matrix4D(const Matrix3D& m)`: the 3x3 goes in the top-left, the 4th row and column are 0, and the corner is 1.

There's no need for a separate file. `Matrix4D.h` already includes `Matrix3D.h`, so the dependency only goes one way (4D knows about 3D, not the reverse), like Matrix3D → Vector3D. `explicit` stops silent conversions, e.g. passing a Matrix3D to a function that takes a Matrix4D.

## Reading list: GPU memory layout (rtfm)

**The concept to learn:** a shader doesn't see a C++ struct. It sees a block whose byte offsets come from a set of layout rules (std140, std430, scalar). Instead of asking "why 16 bytes?", ask "what does each layout rule say a type's alignment is, and which rule applies to the buffer I'm writing?"

1. [Learn Wgpu: Memory Layout in WGSL](https://sotrh.github.io/learn-wgpu/showcase/alignment/): the friendliest explanation of power-of-two alignment and how struct alignment adds up. Read "Alignment of Uniform and Storage buffers". The rules are close to std430.
2. [Vulkan Guide: Shader Memory Layout](https://docs.vulkan.org/guide/latest/shader_memory_layout.html): read "Alignment Requirements", then `VK_EXT_scalar_block_layout`. Work through the four "Alignment Examples" on paper before you look at the answers.
3. [Slang: Handling Matrix Layout Differences](https://docs.shader-slang.org/en/latest/external/slang/docs/user-guide/a1-01-matrix-layout.html): read "Matrix Layout" and "Overriding default matrix layout".
4. [Vulkan spec: Offset and Stride Assignment](https://docs.vulkan.org/spec/latest/chapters/interfaces.html#interfaces-resources-layout): read only "Alignment Requirements" and "Standard Buffer Layout". std430 = base alignment, std140 = extended alignment.
