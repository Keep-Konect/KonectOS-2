#include <kot/sys.h>

__attribute__((section(".KotSpecificData"))) struct KotSpecificData_t KotSpecificData;

KResult Sys_CreateMemoryField(process_t self, size64_t size, uintptr_t* virtualAddressPointer, ksmem_t* keyPointer, enum MemoryFieldType type){
    return Syscall_40(KSys_CreateMemoryField, self, size, virtualAddressPointer, keyPointer, type);
}

KResult Sys_AcceptMemoryField(process_t self, ksmem_t key, uintptr_t* virtualAddressPointer){
    return Syscall_24(KSys_AcceptMemoryField, self, key, virtualAddressPointer);
}

KResult Sys_FreeMemoryField(process_t self, ksmem_t key, uintptr_t address){
    return Syscall_24(KSys_FreeMemoryField, self, key, address);
}

KResult Sys_GetInfoMemoryField(ksmem_t key, uint64_t* typePointer, size64_t* sizePointer){
    return Syscall_24(KSys_GetTypeMemoryField, key, typePointer, sizePointer);
}

KResult Sys_CreateProc(process_t* key, enum Priviledge privilege, uint64_t data){
    return Syscall_24(KSys_CreateProc, key, privilege, data);
}

KResult Sys_CloseProc(){
    return Syscall_0(KSys_CloseProc);
}

KResult SYS_Close(uint64_t errorCode){
    return Syscall_8(KSys_Close, errorCode);
}

KResult SYS_Exit(uint64_t errorCode){
    return Syscall_8(KSys_Exit, errorCode);
}

KResult SYS_Pause(){
    return Syscall_0(KSys_Pause);
}

KResult SYS_Unpause(){
    return Syscall_0(KSys_UnPause);
}

KResult SYS_Map(process_t self, uint64_t* addressVirtual, bool isPhysical, uintptr_t* addressPhysical, size64_t* size, bool findFree){
    return Syscall_48(KSys_Map, self, addressVirtual, isPhysical, addressPhysical, size, findFree);
}

KResult SYS_Unmap(thread_t self, uintptr_t addressVirtual, size64_t size){
    return Syscall_24(KSys_Unmap, self, addressVirtual, size);
}

KResult Sys_Event_Create(kevent_t* self){
    return Syscall_8(KSys_Event_Create, self);
}

KResult Sys_Event_Bind(kevent_t self, thread_t task, uint8_t vector, bool IgnoreMissedEvents){
    return Syscall_32(KSys_Event_Bind, self, task, vector, IgnoreMissedEvents);
}

KResult Sys_Event_Unbind(kevent_t self, thread_t task, uint8_t vector){
    return Syscall_24(KSys_Event_Unbind, self, task, vector);
}

KResult Sys_Event_Trigger(kevent_t self, struct arguments_t* parameters){
    return Syscall_16(KSys_Event_Trigger, self, parameters);
}

KResult Sys_Event_Close(){
    return Syscall_0(KSys_Event_Close);
}

KResult Sys_Createthread(process_t self, uintptr_t entryPoint, enum Priviledge privilege, uint64_t data, thread_t* result){
    return Syscall_40(KSys_CreateThread, self, entryPoint, privilege, data, result);
}

KResult Sys_Duplicatethread(process_t parent, thread_t source, uint64_t data, thread_t* self){
    return Syscall_32(KSys_DuplicateThread, parent, source, data, self);
}

KResult Sys_Execthread(thread_t self, struct arguments_t* parameters, enum ExecutionType type, struct ShareDataWithArguments_t* data){
    return Syscall_32(KSys_ExecThread, self, parameters, type, data);
}

KResult Sys_Keyhole_CloneModify(key_t source, key_t* destination, process_t target, uint64_t flags, enum Priviledge privilidge){
    return Syscall_40(KSys_Keyhole_CloneModify, source, destination, target, flags, privilidge);
}

KResult Sys_Keyhole_Verify(key_t self, enum DataType type, process_t* target, uint64_t* flags, uint64_t* priviledge){
    return Syscall_40(KSys_Keyhole_Verify, self, type, target, flags, priviledge);
}

KResult Sys_Logs(char* message, size64_t size){
    return Syscall_16(KSys_Logs, message, size);
}


KResult Sys_GetthreadKey(thread_t* self){
    /* Get Self Data */
    thread_t key;
    asm("mov %%gs:0, %0":"=r"(key));
    *self = key; 
    return KSUCCESS;
}

KResult Sys_GetProcessKey(process_t* self){
    /* Get Self Data */
    process_t key;
    asm("mov %%gs:8, %0":"=r"(key));
    *self = key; 
    return KSUCCESS;
}

KResult Printlog(char* message){
    return Sys_Logs(message, strlen(message));
}
