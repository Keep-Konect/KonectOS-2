#include <errno.h>
#include <lib/log.h>
#include <lib/math.h>
#include <sys/ioctl.h>
#include <global/heap.h>
#include <global/devfs.h>
#include <global/console.h>
#include <global/scheduler.h>

#define SCANCODE_BUFFER_LENGTH  (512 * 2)

uint64_t scancode_buffer_index_read = 0;
uint64_t scancode_buffer_index_write = 0;
size_t scancode_size_to_read = 0;
uint64_t scancode_buffer[SCANCODE_BUFFER_LENGTH];
spinlock_t key_handler_lock = SPINLOCK_INIT;

void key_handler(uint64_t scancode, uint16_t translated_key, bool is_pressed){
    assert(!spinlock_acquire(&key_handler_lock));

    scancode_buffer[scancode_buffer_index_write] = scancode;
    scancode_buffer[scancode_buffer_index_write + 1] = (((uint64_t)translated_key) | ((uint64_t)is_pressed << 63));
    scancode_buffer_index_write = (scancode_buffer_index_write + 2) % SCANCODE_BUFFER_LENGTH;
    scancode_size_to_read += 2;

    if(scancode_size_to_read > SCANCODE_BUFFER_LENGTH){
        scancode_size_to_read = SCANCODE_BUFFER_LENGTH;
        log_warning("Buffer owerflow\n");
    }

    spinlock_release(&key_handler_lock);
}

int fb_interface_read(void* buffer, size_t size, size_t* bytes_read, struct kernel_file_t* file){
    assert(!spinlock_acquire(&key_handler_lock));

    if(scancode_size_to_read >= 2){
        *((uint64_t*)buffer) = scancode_buffer[scancode_buffer_index_read];
        *((uint64_t*)((uintptr_t)buffer + (uintptr_t)sizeof(uint64_t))) = scancode_buffer[scancode_buffer_index_read + 1];
        scancode_buffer_index_read = (scancode_buffer_index_read + 2) % SCANCODE_BUFFER_LENGTH;

        scancode_size_to_read -= 2;
        *bytes_read = sizeof(uint64_t) * 2;
    }else{
        *bytes_read = 0;
    }

    spinlock_release(&key_handler_lock);
    return 0;
}