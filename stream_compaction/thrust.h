#pragma once

#include "common.h"

namespace StreamCompaction {
    namespace Thrust {
        StreamCompaction::Common::PerformanceTimer& timer();

        void scan(int n, int *odata, const int *idata);
        int compact(int n, int* out, const int* in);
        void sort(int n, int* out, const int* in);
    }
}
