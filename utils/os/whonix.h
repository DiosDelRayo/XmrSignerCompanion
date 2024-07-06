// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2020-2024 The Monero Project

#ifndef FEATHER_WHONIX_H
#define FEATHER_WHONIX_H

#include <QString>

struct WhonixOS {
    static bool detect();
    static QString version();
};


#endif //FEATHER_WHONIX_H
