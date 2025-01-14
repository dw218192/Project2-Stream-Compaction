/**
 * @file      main.cpp
 * @brief     Stream compaction test program
 * @authors   Kai Ninomiya
 * @date      2015
 * @copyright University of Pennsylvania
 */

#include <cstdio>
#include <stream_compaction/cpu.h>
#include <stream_compaction/naive.h>
#include <stream_compaction/efficient.h>
#include <stream_compaction/thrust.h>
#include <stream_compaction/rsort.h>
#include "testing_helpers.hpp"
#include <fstream>

// #define PERFORMANCE_TEST
#ifndef PERFORMANCE_TEST


const int SIZE = 1 << 24; // feel free to change the size of array
const int NPOT = SIZE - 3; // Non-Power-Of-Two
int* a = new int[SIZE];
int* b = new int[SIZE];
int* c = new int[SIZE];

void small_test() {
    constexpr int SMALL_SIZE = 256;
    int a[SMALL_SIZE], b[SMALL_SIZE];
    for (int i = 0; i < SMALL_SIZE; ++i)
        a[i] = i;

    StreamCompaction::CPU::scan(SMALL_SIZE, b, a);
    std::cout << "expected:\n";
    for (int i : b)
        std::cout << i << " ";
    std::cout << std::endl;

    std::cout << "got:\n";
    StreamCompaction::Efficient::scan(SMALL_SIZE, b, a);
    for (int i : b)
        std::cout << i << " ";
    std::cout << std::endl;
}
void sort_test() {
    constexpr int SMALL_SIZE = 8;
#define in a
#define out b
#define correct_out c
    int i = 0;
    for (int x : {4, 7, 2, 6, 3, 5, 1, 0}) {
        in[i++] = x;
    }

    printDesc("gpu sort, power-of-two, small");
    StreamCompaction::Thrust::sort(SMALL_SIZE, correct_out, in);
    StreamCompaction::RadixSort::sort(SMALL_SIZE, out, in);
    printElapsedTime(StreamCompaction::RadixSort::timer().getGpuElapsedTimeForPreviousOperation(), "(std::chrono Measured)");
    printArray(SMALL_SIZE, out);
    printCmpResult(SMALL_SIZE, out, correct_out);

    printDesc("gpu sort, non-power-of-two, small");
    in[i++] = 11;
    printArray(SMALL_SIZE + 1, in);
    StreamCompaction::Thrust::sort(SMALL_SIZE + 1, correct_out, in);
    StreamCompaction::RadixSort::sort(SMALL_SIZE + 1, out, in);
    printElapsedTime(StreamCompaction::RadixSort::timer().getGpuElapsedTimeForPreviousOperation(), "(std::chrono Measured)");
    printArray(SMALL_SIZE + 1, out);
    printCmpResult(SMALL_SIZE + 1, out, correct_out);

    printDesc("gpu sort, power-of-two, large");
    genArray(SIZE, in, 0x3f3f3f3f);
    StreamCompaction::Thrust::sort(SIZE, correct_out, in);
    StreamCompaction::RadixSort::sort(SIZE, out, in);
    printElapsedTime(StreamCompaction::RadixSort::timer().getGpuElapsedTimeForPreviousOperation(), "(std::chrono Measured)");
    printCmpResult(SIZE, out, correct_out);
#undef in
#undef out
#undef correct_out
}


#endif // !PERFORMANCE_TEST

void performance_tests(std::ofstream& fout, int size) {
    int* in = new int[size];
    genArray(size, in, 50);
    int* out = new int[size];
    fout << size << ",";
    StreamCompaction::CPU::scan(size, out, in);
    fout << StreamCompaction::CPU::timer().getCpuElapsedTimeForPreviousOperation() << ",";
    StreamCompaction::Naive::scan(size, out, in);
    fout << StreamCompaction::Naive::timer().getGpuElapsedTimeForPreviousOperation() << ",";
    StreamCompaction::Efficient::scan(size, out, in);
    fout << StreamCompaction::Efficient::timer().getGpuElapsedTimeForPreviousOperation() << ",";
    StreamCompaction::Thrust::scan(size, out, in);
    fout << StreamCompaction::Thrust::timer().getGpuElapsedTimeForPreviousOperation() << "\n";
    delete[] in;
    delete[] out;
}

int main(int argc, char* argv[]) {
    small_test();
    sort_test();
#ifdef PERFORMANCE_TEST
    std::ofstream fout("./data/plot.csv");
    fout << "array size, cpu, naive, efficient, thrust\n";
    int lim = 1 << 30;
    for (int size = 2; size < lim; size <<= 1) {
        performance_tests(fout, size);
        std::cout << size << " ";
    }
    fout.close();
    return 0;
#else
        // Scan tests
    printf("\n");
    printf("****************\n");
    printf("** SCAN TESTS **\n");
    printf("****************\n");

    genArray(SIZE - 1, a, 50);  // Leave a 0 at the end to test that edge case
    a[SIZE - 1] = 0;
    printArray(SIZE, a, true);

    // initialize b using StreamCompaction::CPU::scan you implement
    // We use b for further comparison. Make sure your StreamCompaction::CPU::scan is correct.
    // At first all cases passed because b && c are all zeroes.
    zeroArray(SIZE, b);
    printDesc("cpu scan, power-of-two");
    StreamCompaction::CPU::scan(SIZE, b, a);
    printElapsedTime(StreamCompaction::CPU::timer().getCpuElapsedTimeForPreviousOperation(), "(std::chrono Measured)");
    printArray(SIZE, b, true);

    zeroArray(SIZE, c);
    printDesc("cpu scan, non-power-of-two");
    StreamCompaction::CPU::scan(NPOT, c, a);
    printElapsedTime(StreamCompaction::CPU::timer().getCpuElapsedTimeForPreviousOperation(), "(std::chrono Measured)");
    printArray(NPOT, b, true);
    printCmpResult(NPOT, b, c);

    zeroArray(SIZE, c);
    printDesc("naive scan, power-of-two");
    StreamCompaction::Naive::scan(SIZE, c, a);
    printElapsedTime(StreamCompaction::Naive::timer().getGpuElapsedTimeForPreviousOperation(), "(CUDA Measured)");
    //printArray(SIZE, c, true);
    printCmpResult(SIZE, b, c);

    /* For bug-finding only: Array of 1s to help find bugs in stream compaction or scan
    onesArray(SIZE, c);
    printDesc("1s array for finding bugs");
    StreamCompaction::Naive::scan(SIZE, c, a);
    printArray(SIZE, c, true); */

    zeroArray(SIZE, c);
    printDesc("naive scan, non-power-of-two");
    StreamCompaction::Naive::scan(NPOT, c, a);
    printElapsedTime(StreamCompaction::Naive::timer().getGpuElapsedTimeForPreviousOperation(), "(CUDA Measured)");
    //printArray(SIZE, c, true);
    printCmpResult(NPOT, b, c);

    zeroArray(SIZE, c);
    printDesc("work-efficient scan, power-of-two");
    StreamCompaction::Efficient::scan(SIZE, c, a);
    printElapsedTime(StreamCompaction::Efficient::timer().getGpuElapsedTimeForPreviousOperation(), "(CUDA Measured)");
    //printArray(SIZE, c, true);
    printCmpResult(SIZE, b, c);

    zeroArray(SIZE, c);
    printDesc("work-efficient scan, non-power-of-two");
    StreamCompaction::Efficient::scan(NPOT, c, a);
    printElapsedTime(StreamCompaction::Efficient::timer().getGpuElapsedTimeForPreviousOperation(), "(CUDA Measured)");
    //printArray(NPOT, c, true);
    printCmpResult(NPOT, b, c);

    zeroArray(SIZE, c);
    printDesc("thrust scan, power-of-two");
    StreamCompaction::Thrust::scan(SIZE, c, a);
    printElapsedTime(StreamCompaction::Thrust::timer().getGpuElapsedTimeForPreviousOperation(), "(CUDA Measured)");
    //printArray(SIZE, c, true);
    printCmpResult(SIZE, b, c);

    zeroArray(SIZE, c);
    printDesc("thrust scan, non-power-of-two");
    StreamCompaction::Thrust::scan(NPOT, c, a);
    printElapsedTime(StreamCompaction::Thrust::timer().getGpuElapsedTimeForPreviousOperation(), "(CUDA Measured)");
    //printArray(NPOT, c, true);
    printCmpResult(NPOT, b, c);

    printf("\n");
    printf("*****************************\n");
    printf("** STREAM COMPACTION TESTS **\n");
    printf("*****************************\n");

    // Compaction tests

    genArray(SIZE - 1, a, 4);  // Leave a 0 at the end to test that edge case
    a[SIZE - 1] = 0;
    printArray(SIZE, a, true);

    int count, expectedCount, expectedNPOT;

    // initialize b using StreamCompaction::CPU::compactWithoutScan you implement
    // We use b for further comparison. Make sure your StreamCompaction::CPU::compactWithoutScan is correct.
    zeroArray(SIZE, b);
    printDesc("cpu compact without scan, power-of-two");
    count = StreamCompaction::CPU::compactWithoutScan(SIZE, b, a);
    printElapsedTime(StreamCompaction::CPU::timer().getCpuElapsedTimeForPreviousOperation(), "(std::chrono Measured)");
    expectedCount = count;
    printArray(count, b, true);
    printCmpLenResult(count, expectedCount, b, b);

    zeroArray(SIZE, c);
    printDesc("cpu compact without scan, non-power-of-two");
    count = StreamCompaction::CPU::compactWithoutScan(NPOT, c, a);
    printElapsedTime(StreamCompaction::CPU::timer().getCpuElapsedTimeForPreviousOperation(), "(std::chrono Measured)");
    expectedNPOT = count;
    printArray(count, c, true);
    printCmpLenResult(count, expectedNPOT, b, c);

    zeroArray(SIZE, c);
    printDesc("cpu compact with scan");
    count = StreamCompaction::CPU::compactWithScan(SIZE, c, a);
    printElapsedTime(StreamCompaction::CPU::timer().getCpuElapsedTimeForPreviousOperation(), "(std::chrono Measured)");
    printArray(count, c, true);
    printCmpLenResult(count, expectedCount, b, c);

    zeroArray(SIZE, c);
    printDesc("work-efficient compact, power-of-two");
    count = StreamCompaction::Efficient::compact(SIZE, c, a);
    printElapsedTime(StreamCompaction::Efficient::timer().getGpuElapsedTimeForPreviousOperation(), "(CUDA Measured)");
    //printArray(count, c, true);
    printCmpLenResult(count, expectedCount, b, c);

    zeroArray(SIZE, c);
    printDesc("work-efficient compact, non-power-of-two");
    count = StreamCompaction::Efficient::compact(NPOT, c, a);
    printElapsedTime(StreamCompaction::Efficient::timer().getGpuElapsedTimeForPreviousOperation(), "(CUDA Measured)");
    //printArray(count, c, true);
    printCmpLenResult(count, expectedNPOT, b, c);

    zeroArray(SIZE, c);
    printDesc("thrust compact, power-of-two");
    count = StreamCompaction::Thrust::compact(SIZE, c, a);
    printElapsedTime(StreamCompaction::Thrust::timer().getGpuElapsedTimeForPreviousOperation(), "(CUDA Measured)");
    //printArray(count, c, true);
    printCmpLenResult(count, expectedCount, b, c);

    zeroArray(SIZE, c);
    printDesc("thrust compact, non-power-of-two");
    count = StreamCompaction::Thrust::compact(NPOT, c, a);
    printElapsedTime(StreamCompaction::Thrust::timer().getGpuElapsedTimeForPreviousOperation(), "(CUDA Measured)");
    //printArray(count, c, true);
    printCmpLenResult(count, expectedNPOT, b, c);

    system("pause"); // stop Win32 console from closing on exit
    delete[] a;
    delete[] b;
    delete[] c;
#endif // PERFORMANCE_TEST
}
