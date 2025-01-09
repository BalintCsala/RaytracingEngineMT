glslc raytracing/firstray.rgen -o ../shaders-spv/raytracing/firstray.rgen.spv --target-env=vulkan1.3

glslc gltf.rgen -o ../shaders-spv/gltf.rgen.spv --target-env=vulkan1.3
glslc gltf.rchit -o ../shaders-spv/gltf.rchit.spv --target-env=vulkan1.3
glslc gltf.rahit -o ../shaders-spv/gltf.rahit.spv --target-env=vulkan1.3
glslc gltf.rmiss -o ../shaders-spv/gltf.rmiss.spv --target-env=vulkan1.3

glslc compute/accumulate.comp -o ../shaders-spv/compute/accumulate.comp.spv --target-env=vulkan1.3
glslc compute/sky.comp -o ../shaders-spv/compute/sky.comp.spv --target-env=vulkan1.3
glslc compute/copy_prev.comp -o ../shaders-spv/compute/copy_prev.comp.spv --target-env=vulkan1.3
glslc compute/tonemap.comp -o ../shaders-spv/compute/tonemap.comp.spv --target-env=vulkan1.3
