#pragma once

#include <sys/stat.h>

#include <cstring>
#include <eaio.hpp>
#include <unistd.h>

namespace ervan::io {
    template <size_t N>
    void tar_write(char (&buffer)[N], char value) {
        buffer[N - 1] = value;
    }

    template <size_t N>
    void tar_write(char (&buffer)[N], size_t value) {
        for (int i = 0; i < N - 1; i++) {
            buffer[N - 2 - i] = '0' + (value % 8);
            value /= 8;
        }

        buffer[N - 1] = '\0';
    }

    template <size_t N>
    void tar_write(char (&buffer)[N], const char* value) {
        std::strncpy(buffer, value, N - 1);

        for (size_t i = strlen(buffer); i < N; i++)
            buffer[i] = '\0';

        buffer[N - 1] = '\0';
    }

    template <size_t N, typename T>
    void tar_write(char (&buffer)[N], T value) {
        tar_write(buffer, static_cast<size_t>(value));
    }

    struct tar_header {
        char path0[100];
        char mode[8];
        char owner[8];
        char group[8];
        char file_size[12];
        char last_modification[12];
        char checksum[7];
        char checksum_space[1];
        char file_type[1];
        char linked_name[100];
        char ustar_magic[8];
        char owner_name[32];
        char group_name[32];
        char device_major[8];
        char device_minor[8];
        char filename_prefix[155];
        char zero[12];

        static tar_header filled(const char* name, size_t file_size) {
            tar_header hdr;

            // memset(hdr.zero, sizeof(hdr.zero), '\0');
            tar_write(hdr.path0, name);
            tar_write(hdr.mode, S_IRUSR | S_IWUSR | S_IRGRP);
            tar_write(hdr.owner, 0);
            tar_write(hdr.group, 0);
            tar_write(hdr.file_size, file_size);
            tar_write(hdr.last_modification, time(nullptr));
            tar_write(hdr.checksum, "        ");
            tar_write(hdr.checksum_space, ' ');
            tar_write(hdr.file_type, '0');
            tar_write(hdr.linked_name, "");
            tar_write(hdr.ustar_magic, "ustar  ");
            tar_write(hdr.owner_name, "ervan");
            tar_write(hdr.group_name, "ervan");
            tar_write(hdr.device_major, "");
            tar_write(hdr.device_minor, "");
            tar_write(hdr.filename_prefix, "");
            tar_write(hdr.zero, "");

            hdr.checksum[6] = ' ';

            size_t sum = 0;

            for (int i = 0; i < sizeof(hdr); i++)
                sum += reinterpret_cast<uint8_t*>(&hdr)[i];

            tar_write(hdr.checksum, sum);

            return hdr;
        }
    } __attribute((packed));

    static_assert(sizeof(tar_header) == 512);

    void            install_io_signal();
    eaio::coro<int> sync_file(eaio::file& file);
}