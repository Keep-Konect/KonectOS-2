#pragma once

#include <core/core.h>
#include <stdio.h>

typedef KResult (*file_dispatch_t)(thread_t Callback, uint64_t CallbackArg, struct ext_file_t* File, uint64_t GP0, uint64_t GP1, uint64_t GP2);
typedef KResult (*dir_dispatch_t)(thread_t Callback, uint64_t CallbackArg, struct ext_directory_t* Directory, uint64_t GP0, uint64_t GP1, uint64_t GP2);


KResult MountToVFS(struct mount_info_t* MountInfo, process_t VFSProcess, thread_t VFSMountThread);

KResult ChangeUserData(thread_t Callback, uint64_t CallbackArg, uint64_t UID, uint64_t GID, char* UserName);

KResult Removefile(thread_t Callback, uint64_t CallbackArg, char* Path, kot_permissions_t Permissions);

KResult Openfile(thread_t Callback, uint64_t CallbackArg, char* Path, kot_permissions_t Permissions, process_t Target);

KResult FileDispatch(thread_t Callback, uint64_t CallbackArg, uint64_t GP0, uint64_t GP1, uint64_t GP2, uint64_t GP3);
KResult Closefile(thread_t Callback, uint64_t CallbackArg, ext_file_t* File, uint64_t GP0, uint64_t GP1, uint64_t GP2);
KResult Getfilesize(thread_t Callback, uint64_t CallbackArg, ext_file_t* File, uint64_t GP0, uint64_t GP1, uint64_t GP2);
KResult Readfile(thread_t Callback, uint64_t CallbackArg, ext_file_t* File, uint64_t GP0, uint64_t GP1, uint64_t GP2);
KResult Writefile(thread_t Callback, uint64_t CallbackArg, ext_file_t* File, uint64_t GP0, uint64_t GP1, uint64_t GP2);

KResult Rename(thread_t Callback, uint64_t CallbackArg, struct srv_storage_fs_server_rename_t* RenameData, kot_permissions_t Permissions);

KResult Mkdir(thread_t Callback, uint64_t CallbackArg, char* Path, mode_t Mode, kot_permissions_t Permissions);

KResult Rmdir(thread_t Callback, uint64_t CallbackArg, char* Path, kot_permissions_t Permissions);

KResult Opendir(thread_t Callback, uint64_t CallbackArg, char* Path, kot_permissions_t Permissions, process_t Target);

KResult DirDispatch(thread_t Callback, uint64_t CallbackArg, uint64_t GP0, uint64_t GP1, uint64_t GP2, uint64_t GP3);
KResult Readdir(thread_t Callback, uint64_t CallbackArg, ext_directory_t* Directory, uint64_t GP0, uint64_t GP1, uint64_t GP2);
KResult Getdircount(thread_t Callback, uint64_t CallbackArg, ext_directory_t* Directory, uint64_t GP0, uint64_t GP1, uint64_t GP2);
KResult Closedir(thread_t Callback, uint64_t CallbackArg, ext_directory_t* Directory, uint64_t GP0, uint64_t GP1, uint64_t GP2);