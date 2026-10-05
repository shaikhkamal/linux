/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_MEMORY_FAILURE_H
#define _LINUX_MEMORY_FAILURE_H

#include <linux/interval_tree.h>

struct pfn_address_space {
	struct interval_tree_node node;
	struct address_space *mapping;
	int (*pfn_to_vma_pgoff)(struct vm_area_struct *vma,
				unsigned long pfn, pgoff_t *pgoff);
};

struct preonline_hwpoison;

/**
 * struct preonline_hwpoison_range - Pre-online poisoned PFN range
 * @start_pfn: First PFN in the range
 * @nr_pages: Number of consecutive PFNs in the range
 */
struct preonline_hwpoison_range {
	unsigned long start_pfn;
	unsigned long nr_pages;
};

#ifdef CONFIG_MEMORY_FAILURE
int register_pfn_address_space(struct pfn_address_space *pfn_space);
void unregister_pfn_address_space(struct pfn_address_space *pfn_space);

/**
 * preonline_hwpoison_register - Register poison before memory is onlined
 * @ranges: Sorted, nonoverlapping PFN ranges in ascending start PFN order.
 *          Adjacent ranges are accepted, but callers should coalesce them.
 *          The array is copied and need not outlive this call.
 * @nr_ranges: Number of entries in @ranges. Must be nonzero.
 * @handle: Registration handle returned on success. Set to NULL on failure.
 *
 * Register PFN ranges that must be withheld when their backing memory is
 * initialized. Pass the returned handle to
 * preonline_hwpoison_unregister() only after the memory can no longer enter
 * the allocator.
 *
 * Return: 0 on success, -EINVAL if @handle is NULL or @ranges is empty,
 * unsorted, overlapping, or contains a zero-length entry, -EOVERFLOW on PFN
 * wraparound, and -ENOMEM on allocation failure.
 */
int preonline_hwpoison_register(const struct preonline_hwpoison_range *ranges,
				unsigned int nr_ranges,
				struct preonline_hwpoison **handle);

/**
 * preonline_hwpoison_unregister - Remove a pre-online poison registration
 * @handle: Handle returned by preonline_hwpoison_register(), or NULL
 *
 * Remove @handle from future pre-online poison lookups. The caller must first
 * ensure that the associated memory can no longer be initialized or handed
 * to the page allocator.
 *
 * Pages already marked as hwpoison remain marked.
 */
void preonline_hwpoison_unregister(struct preonline_hwpoison *handle);

/**
 * preonline_hwpoison_intersects - Does a PFN range cover a known-bad frame?
 * @start_pfn: First PFN of the range to test
 * @nr_pages: Length of the range in frames
 *
 * Return: %true if any registered range overlaps [@start_pfn, @start_pfn +
 * @nr_pages), %false otherwise.
 */
bool preonline_hwpoison_intersects(unsigned long start_pfn,
				   unsigned long nr_pages);

/**
 * preonline_hwpoison_contains - Is a single frame known bad?
 * @pfn: Frame to test
 *
 * Return: %true if @pfn falls in a registered range.
 */
bool preonline_hwpoison_contains(unsigned long pfn);
#else
static inline int register_pfn_address_space(struct pfn_address_space *pfn_space)
{
	return -EOPNOTSUPP;
}

static inline void unregister_pfn_address_space(struct pfn_address_space *pfn_space)
{
}

static inline int
preonline_hwpoison_register(const struct preonline_hwpoison_range *ranges,
			    unsigned int nr_ranges,
			    struct preonline_hwpoison **handle)
{
	if (handle)
		*handle = NULL;

	return -EOPNOTSUPP;
}

static inline void
preonline_hwpoison_unregister(struct preonline_hwpoison *handle)
{
}

static inline bool preonline_hwpoison_intersects(unsigned long start_pfn,
						 unsigned long nr_pages)
{
	return false;
}

static inline bool preonline_hwpoison_contains(unsigned long pfn)
{
	return false;
}

#endif /* CONFIG_MEMORY_FAILURE */

#endif /* _LINUX_MEMORY_FAILURE_H */
