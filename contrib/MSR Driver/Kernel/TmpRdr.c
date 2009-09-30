#include <ntddk.h>

#define FILE_DEVICE_UNKNOWN             0x00000022
#define IOCTL_UNKNOWN_BASE              FILE_DEVICE_UNKNOWN
#define IOCTL_PROCVIEW_RDMSR			CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0803, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define FLAG_HANDLE_OPENED 1

void UnloadDriver(PDRIVER_OBJECT DriverObject);
NTSTATUS DispatchCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS DispatchIoctl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);

typedef struct _DEVICE_EXTENSION{
    PDEVICE_OBJECT DeviceObject;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

PDEVICE_OBJECT g_pDeviceObject;

#pragma alloc_text(PAGE0DEF, DriverEntry) 

//
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath){
    NTSTATUS        ntStatus;
    UNICODE_STRING  uszDriverString;
    UNICODE_STRING  uszDeviceString;

    PDEVICE_OBJECT    pDeviceObject;
    PDEVICE_EXTENSION extension;
    
	// Point uszDriverString at the driver name
    RtlInitUnicodeString(&uszDriverString, L"\\Device\\TmpRdr");

    // Create and initialize device object
    ntStatus = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION), &uszDriverString, FILE_DEVICE_UNKNOWN, 0, FALSE, &pDeviceObject);
    if(ntStatus != STATUS_SUCCESS)
        return ntStatus;
    
	// Assign extension variable
    extension = pDeviceObject->DeviceExtension;
    
	// Point uszDeviceString at the device name
    RtlInitUnicodeString(&uszDeviceString, L"\\DosDevices\\TmpRdr");
    
	// Create symbolic link to the user-visible name
    ntStatus = IoCreateSymbolicLink(&uszDeviceString, &uszDriverString);

    if(ntStatus != STATUS_SUCCESS){
        // Delete device object if not successful
        IoDeleteDevice(pDeviceObject);
        return ntStatus;
    }

    // Assign global pointer to the device object for use by the callback functions
    g_pDeviceObject = pDeviceObject;

    // Load structure to point to IRP handlers
    DriverObject->DriverUnload                         = UnloadDriver;
    DriverObject->MajorFunction[IRP_MJ_CREATE]         = DispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = DispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoctl;

    // Return success
    return ntStatus;
}

//
NTSTATUS DispatchCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp){
	PDEVICE_EXTENSION     extension = DeviceObject->DeviceExtension;

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information  = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

//
NTSTATUS DispatchIoctl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp){
    NTSTATUS              ntStatus = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION    irpStack  = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION     extension = DeviceObject->DeviceExtension;
	__int64				  *p__int64;
	int					  iMSRregister;
  
    switch(irpStack->Parameters.DeviceIoControl.IoControlCode){
		case IOCTL_PROCVIEW_RDMSR:
			if(irpStack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(__int64)){
				if(irpStack->Parameters.DeviceIoControl.InputBufferLength == sizeof(int))
					iMSRregister = *((int *)Irp->AssociatedIrp.SystemBuffer);
				else
					iMSRregister = 0x19c;

				p__int64 = Irp->AssociatedIrp.SystemBuffer;
				*p__int64 = __readmsr(iMSRregister);

                ntStatus = STATUS_SUCCESS;
				Irp->IoStatus.Information = sizeof(__int64);
				IoCompleteRequest(Irp, IO_NO_INCREMENT);
				return ntStatus;
            }
			break;
        default:
            break;
    }

    Irp->IoStatus.Status = ntStatus;
   
    if(ntStatus == STATUS_SUCCESS)
        Irp->IoStatus.Information = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    else
        Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return ntStatus;
}

//
void UnloadDriver(IN PDRIVER_OBJECT DriverObject){
    UNICODE_STRING  uszDeviceString;

    IoDeleteDevice(DriverObject->DeviceObject);

    RtlInitUnicodeString(&uszDeviceString, L"\\DosDevices\\TmpRdr");
    IoDeleteSymbolicLink(&uszDeviceString);
}
