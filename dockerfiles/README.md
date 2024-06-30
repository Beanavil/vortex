# Dockerfiles

One docker image is shipped with our Vortex, which builds and installs the modified LLVM and then builds Vortex. For building the image, follow these instructions:

1. Create a directory (`/path/to/dir`) and copy the `vortex-20240630.Dockerfile` from this folder.
2. Rename the file to `Dockerfile`.
3. Build the image with Docker:
    ```bash
    docker build -t vortexgpgpu/vortex:xohc /path/to/dir
    ```