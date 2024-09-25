#include <libproc.h>
#include <simd/simd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define assert(b) \
	if (!(b)) \
	{ \
		__builtin_debugtrap(); \
	}

static ptrdiff_t
cycle_count(void)
{
	struct rusage_info_v6 rusage = {0};
	int32_t status = proc_pid_rusage(getpid(), RUSAGE_INFO_V6, (rusage_info_t)&rusage);
	assert(status == 0);
	return rusage.ri_cycles;
}

static double
scalar(double *data, ptrdiff_t count)
{
	double result = 0;
	for (ptrdiff_t i = 0; i < count; i++)
	{
		result += data[i];
	}
	return result;
}

static double
simd2x1_no_align(double *data, ptrdiff_t count)
{
	assert(count % 2 == 0);
	simd_double2 acc = 0;
	for (ptrdiff_t i = 0; i < count; i += 2)
	{
		acc += *(simd_double2 *)(data + i);
	}
	return simd_reduce_add(acc);
}

static double
simd8x1_no_align(double *data, ptrdiff_t count)
{
	assert(count % 8 == 0);
	simd_double8 acc = 0;
	for (ptrdiff_t i = 0; i < count; i += 8)
	{
		acc += *(simd_double8 *)(data + i);
	}
	return simd_reduce_add(acc);
}

static double
simd8x4_no_align(double *data, ptrdiff_t count)
{
	assert(count % (8 * 4) == 0);
	simd_double8 acc0 = 0;
	simd_double8 acc1 = 0;
	simd_double8 acc2 = 0;
	simd_double8 acc3 = 0;
	for (ptrdiff_t i = 0; i < count; i += 8 * 4)
	{
		simd_double8 *p = (simd_double8 *)(data + i);
		acc0 += p[0];
		acc1 += p[1];
		acc2 += p[2];
		acc3 += p[3];
	}
	return simd_reduce_add(acc0) + simd_reduce_add(acc1) + simd_reduce_add(acc2) +
	       simd_reduce_add(acc3);
}

static void
run(char *name,
        double (*impl)(double *, ptrdiff_t),
        double *data,
        ptrdiff_t count,
        double expected_sum)
{
	ptrdiff_t run_count = 10000;
	double *run_results = calloc(run_count, sizeof(double));

	ptrdiff_t before = cycle_count();
	for (ptrdiff_t i = 0; i < run_count; i++)
	{
		run_results[i] = impl(data, count);
	}
	ptrdiff_t after = cycle_count();

	for (ptrdiff_t i = 0; i < run_count; i++)
	{
		assert(fabs(run_results[i] - expected_sum) < 0.001f);
	}

	ptrdiff_t cycles = after - before;

	printf("%20s: %4.f Mcycles, %.3f cycles/element, %.3f elements/cycle\n", name,
	        (double)cycles / 1000000, (double)cycles / (double)(count * run_count),
	        (double)(count * run_count) / (double)cycles);
}

int32_t
main(void)
{
	ptrdiff_t count = 64 * 1024;
	double *data = calloc(count, sizeof(double));
	double expected_sum = 0;
	for (ptrdiff_t i = 0; i < count; i++)
	{
		double element = (double)arc4random_uniform(1000000) / 1000000;
		expected_sum += element;
		data[i] = element;
	}

	run("scalar", scalar, data, count, expected_sum);
	run("simd2x1_no_align", simd2x1_no_align, data, count, expected_sum);
	run("simd8x1_no_align", simd8x1_no_align, data, count, expected_sum);
	run("simd8x4_no_align", simd8x4_no_align, data, count, expected_sum);
}
