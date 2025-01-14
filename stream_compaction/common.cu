#include "common.h"

void checkCUDAErrorFn(const char *msg, const char *file, int line) {
    cudaError_t err = cudaGetLastError();
    if (cudaSuccess == err) {
        return;
    }

    fprintf(stderr, "CUDA error");
    if (file) {
        fprintf(stderr, " (%s:%d)", file, line);
    }
    fprintf(stderr, ": %s: %s\n", msg, cudaGetErrorString(err));
    exit(EXIT_FAILURE);
}


namespace StreamCompaction {
    namespace Common {
        /**
         * makes an array have length that is a power of two
         * returns the new length and puts the result in *out
         */
        int makePowerTwoLength(int n, int const* in, int** out, MakePowerTwoLengthMode mode) {
            int old_len = n;
            if (n == 1 || ((n & -n) != n)) {
                // n is not a power of two
                n = 1 << ilog2ceil(n);
            }
            // don't alloc if the buffer is not null
            if (!*out) {
                cudaMalloc(out, n * sizeof(int));
            }
            if (mode == HostToDevice) {
                cudaMemcpy(*out, in, old_len * sizeof(int), cudaMemcpyHostToDevice);
            } else if (mode == DeviceToDevice) {
                cudaMemcpy(*out, in, old_len * sizeof(int), cudaMemcpyDeviceToDevice);
            }
            if (n != old_len) {
                cudaMemset(*out + old_len, 0, (n - old_len) * sizeof(int));
            }
            return n;
        }

        /**
         * Maps an array to an array of 0s and 1s for stream compaction. Elements
         * which map to 0 will be removed, and elements which map to 1 will be kept.
         */
        __global__ void kernMapToBoolean(int n, int *bools, const int *idata) {
            // TODO
            int self = (blockIdx.x * blockDim.x) + threadIdx.x;
            if (self < n) {
                bools[self] = idata[self] ? 1 : 0;
            }
        }

        /**
         * Performs scatter on an array. That is, for each element in idata,
         * if bools[idx] == 1, it copies idata[idx] to odata[indices[idx]].
         */
        __global__ void kernScatter(int n, int *odata,
                const int *idata, const int *bools, const int *indices) {
            // TODO
            int self = (blockIdx.x * blockDim.x) + threadIdx.x;
            if (self < n && bools[self]) {
                odata[indices[self]] = idata[self];
            }
        }

    }
}
