/*
 * cais - Communication Abstraction & I/O Substrate
 * Projectum Lunare <https://github.com/projectum-lunare/cais>
 * Copyright (C) 2026 Gustavo Veiga <gustavo.veiga@outlook.com.br>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef CAIS_H
#define CAIS_H

#ifndef CAIS_NO_PLATFORM
#include <cais/platform.h>
#endif

#ifndef CAIS_NO_FILESYSTEM
#include <cais/filesystem.h>
#include <cais/fs.h>
#endif

#ifndef CAIS_NO_PARALLEL
#include <cais/parallel.h>
#endif

#ifndef CAIS_NO_SERIAL
#include <cais/serial.h>
#endif

#ifndef CAIS_NO_SOCKET
#include <cais/socket.h>
#endif

#ifndef CAIS_NO_SANDBOX
#include <cais/sandbox.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define CAIS_VERSION_MAJOR 0
#define CAIS_VERSION_MINOR 2
#define CAIS_VERSION_PATCH 0
#define CAIS_VERSION_STRING "0.3.0"

/*
 * Returns the version string of the cais runtime foundation.
 */
const char *cais_version(void);

#ifdef __cplusplus
}
#endif

#endif /* CAIS_H */
