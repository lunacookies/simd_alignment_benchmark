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

static void
run(char *name,
        double (*impl)(double *, ptrdiff_t),
        double *data,
        ptrdiff_t count,
        double expected_sum)
{
	ptrdiff_t run_count = 1000;
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

	printf("%20s: %4.f Mcycles, %.2f cycles/element, %.2f elements/cycle\n", name,
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
}
