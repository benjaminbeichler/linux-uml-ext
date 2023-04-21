/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_AVERAGE_H
#define _LINUX_AVERAGE_H

#include <linux/compiler.h>
#include <linux/log2.h>

/*
 * Exponentially weighted moving average (EWMA)
 *
 * This implements a fixed-precision EWMA algorithm, with both the
 * precision and fall-off coefficient determined at compile-time
 * and built into the generated helper funtions.
 *
 * The first argument to the macro is the name that will be used
 * for the struct and helper functions.
 *
 * The second argument, the precision, expresses how many bits are
 * used for the fractional part of the fixed-precision values.
 *
 * The third argument, the weight reciprocal, determines how the
 * new values will be weighed vs. the old state, new values will
 * get weight 1/weight_rcp and old values 1-1/weight_rcp. Note
 * that this parameter must be a power of two for efficiency. A
 * weight of 1 is not allowed, because of the internal init function,
 * but nevertheless it makes no sense, since it means no averaging at all.
 */

#define EWMA_BUILD_TIME_CHECKS(_precision, _weight_rcp)			\
	do {								\
		BUILD_BUG_ON(!__builtin_constant_p(_precision));	\
		BUILD_BUG_ON(!__builtin_constant_p(_weight_rcp));	\
									\
		BUILD_BUG_ON((_precision) > 30);			\
		BUILD_BUG_ON_NOT_POWER_OF_2(_weight_rcp);		\
		BUILD_BUG_ON(_weight_rcp <= 1);				\
	} while (0)

#define DECLARE_EWMA(name, _precision, _weight_rcp)				\
	struct ewma_##name {							\
		unsigned long internal;						\
	};									\
	static inline void ewma_##name##_init(struct ewma_##name *e)		\
	{									\
		EWMA_BUILD_TIME_CHECKS(_precision, _weight_rcp);		\
		e->internal = ULONG_MAX;					\
	}									\
	static inline void ewma_##name##_init_val(struct ewma_##name *e,	\
						unsigned long val)		\
	{									\
		EWMA_BUILD_TIME_CHECKS(_precision, _weight_rcp);		\
		e->internal = val;						\
	}									\
	static inline unsigned long						\
	ewma_##name##_read(struct ewma_##name *e)				\
	{									\
		EWMA_BUILD_TIME_CHECKS(_precision, _weight_rcp);		\
		return unlikely(e->internal == ULONG_MAX) ?			\
				0 : e->internal >> (_precision);		\
	}									\
	static inline void ewma_##name##_add(struct ewma_##name *e,		\
					     unsigned long val)			\
	{									\
		unsigned long internal = READ_ONCE(e->internal);		\
		unsigned long weight_rcp = ilog2(_weight_rcp);			\
		unsigned long precision = _precision;				\
		unsigned long max_val = ULONG_MAX >> (precision + weight_rcp);	\
										\
		EWMA_BUILD_TIME_CHECKS(_precision, _weight_rcp);		\
		WARN_ONCE(val > max_val, "Value (%lu) is too large for ewma_##name##_add, this will result in unexpected values of the ewma filter. Max valid value is %lu",\
			  val, max_val);					\
		WRITE_ONCE(e->internal, unlikely(internal != ULONG_MAX) ?	\
			(((internal << weight_rcp) - internal) +		\
				(val << precision)) >> weight_rcp :		\
			(val << precision));					\
	}

#endif /* _LINUX_AVERAGE_H */
