// MySimpleDrv.c  (KMDF kernel-mode driver, C)
// Build with: Visual Studio + WDK (KMDF driver project)

#include <ntddk.h>
#include <wdf.h>

#define DEVICE_NAME      L"\\Device\\MySimpleDrv"
#define SYMLINK_NAME     L"\\DosDevices\\MySimpleDrv"

// Custom IOCTL: GET_VERSION (METHOD_BUFFERED, any access)
// In production: use tighter access (e.g., FILE_READ_DATA or admin-only device ACL)
#define IOCTL_MYSIMPLEDRV_GET_VERSION CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD EvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL EvtIoDeviceControl;

static const char g_Version[] = "MySimpleDrv v1.0";

_Use_decl_annotations_
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    WDF_DRIVER_CONFIG config;
    WDF_DRIVER_CONFIG_INIT(&config, EvtDeviceAdd);

    // Optional: add WPP tracing, etc. For now, keep minimal.
    return WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);
}

_Use_decl_annotations_
NTSTATUS EvtDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit)
{
    UNREFERENCED_PARAMETER(Driver);

    NTSTATUS status;
    WDFDEVICE device;
    WDF_IO_QUEUE_CONFIG queueConfig;

    // We expose a named device so user-mode can open \\.\MySimpleDrv
    DECLARE_CONST_UNICODE_STRING(devName, DEVICE_NAME);
    DECLARE_CONST_UNICODE_STRING(symLink, SYMLINK_NAME);

    status = WdfDeviceInitAssignName(DeviceInit, &devName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Create device object
    status = WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &device);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Create symbolic link \\.\MySimpleDrv
    status = WdfDeviceCreateSymbolicLink(device, &symLink);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Default I/O queue (sequential, simplest)
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchSequential);
    queueConfig.EvtIoDeviceControl = EvtIoDeviceControl;

    status = WdfIoQueueCreate(device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, WDF_NO_HANDLE);
    return status;
}

_Use_decl_annotations_
VOID EvtIoDeviceControl(
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength,
    ULONG IoControlCode
)
{
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(InputBufferLength);

    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    size_t bytesToCopy = 0;

    if (IoControlCode == IOCTL_MYSIMPLEDRV_GET_VERSION) {

        // METHOD_BUFFERED: output is in the request’s output buffer
        // We return a C-string including the trailing '\0'.
        bytesToCopy = sizeof(g_Version); // includes '\0'

        if (OutputBufferLength < bytesToCopy) {
            status = STATUS_BUFFER_TOO_SMALL;
            bytesToCopy = 0;
        } else {
            PVOID outBuf = NULL;
            size_t outBufSize = 0;

            status = WdfRequestRetrieveOutputBuffer(Request, bytesToCopy, &outBuf, &outBufSize);
            if (NT_SUCCESS(status)) {
                RtlCopyMemory(outBuf, g_Version, bytesToCopy);
            } else {
                bytesToCopy = 0;
            }
        }

        WdfRequestCompleteWithInformation(Request, status, bytesToCopy);
        return;
    }

    // Unknown IOCTL
    WdfRequestCompleteWithInformation(Request, status, 0);
}
