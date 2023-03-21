#ifndef OPEN_SIMPLEX_NOISE_H__
#define OPEN_SIMPLEX_NOISE_H__

/*
 * OpenSimplex (Simplectic) Noise in C.
 * Ported to C from Kurt Spencer's java implementation by Stephen M. Cameron
 *
 * v1.1 (October 6, 2014) 
 * - Ported to C
 * 
 * v1.1 (October 5, 2014)
 * - Added 2D and 4D implementations.
 * - Proper gradient sets for all dimensions, from a
 *   dimensionally-generalizable scheme with an actual
 *   rhyme and reason behind it.
 * - Removed default permutation array in favor of
 *   default seed.
 * - Changed seed-based constructor to be independent
 *   of any particular randomization library, so results
 *   will be the same when ported to other languages.
 */

#if ((__GNUC_STDC_INLINE__) || (__STDC_VERSION__ >= 199901L))
	#include <stdint.h>
	#define INLINE inline
#elif (defined (_MSC_VER) || defined (__GNUC_GNU_INLINE__))
	#include <stdint.h>
	#define INLINE __inline
#else 
	/* ANSI C doesn't have inline or stdint.h. */
	#define INLINE
#endif

#ifdef __cplusplus
	extern "C" {
#endif

typedef struct osn_context* SimplexContext;

SimplexContext simplex_context_create(int64_t seed);
void simplex_context_destroy(SimplexContext);
double simplex_sample3(SimplexContext, double x, double y, double z);

#ifdef __cplusplus
	}
#endif

#endif
