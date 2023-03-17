clang -framework OpenCL main.c -o m

xcode duplicate -> double click proj -> build phases -> compile sources


ls /System/Library/Frameworks/OpenCL.framework

sudo cp ~/project_x/opencl/cl.hpp /System/Library/Frameworks/OpenCL.framework/Headers

bottom left setting icon -apply to enclosed items
diskutil resetUserPermissions / `id -u`
