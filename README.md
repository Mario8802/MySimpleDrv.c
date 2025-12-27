## MySimpleDrv

Minimal KMDF kernel-mode Windows driver written in C.

### Features
- Creates device \\.\MySimpleDrv
- Handles IOCTL_MYSIMPLEDRV_GET_VERSION
- Uses METHOD_BUFFERED
- Demonstrates safe IRP handling

### Purpose
Educational driver for understanding:
- IRP flow
- IOCTL handling
- Kernel/user boundary

### NOT intended for production use
