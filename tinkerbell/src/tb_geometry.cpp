// ================================================================================
// == This file is a part of TinkerBell UI Toolkit. (C) 2011-2013, Emil Segerås  ==
// ==                   See tinkerbell.h for more information.                   ==
// ================================================================================

#include "tb_geometry.h"
#include <assert.h>

namespace tinkerbell {

// == TBRegion ====================================================================

TBRegion::TBRegion()
	: m_rects(nullptr)
	, m_num_rects(0)
	, m_capacity(0)
{
}

TBRegion::~TBRegion()
{
	RemoveAll(true);
}

void TBRegion::RemoveRect(int index)
{
	assert(index >= 0 && index < m_num_rects);
	for (int i = index; i < m_num_rects - 1; i++)
		m_rects[i] = m_rects[i + 1];
	m_num_rects--;
}

void TBRegion::RemoveRectFast(int index)
{
	assert(index >= 0 && index < m_num_rects);
	m_rects[index] = m_rects[--m_num_rects];
}

void TBRegion::RemoveAll(bool free_memory)
{
	m_num_rects = 0;
	if (free_memory)
	{
		delete [] m_rects;
		m_rects = nullptr;
		m_capacity = 0;
	}
}

bool TBRegion::Set(const TBRect &rect)
{
	RemoveAll();
	return AddRect(rect, false);
}

bool TBRegion::GrowIfNeeded()
{
	if (m_num_rects == m_capacity)
	{
		int new_m_capacity = CLAMP(4, m_capacity * 2, 1024);
		TBRect *new_rects = new TBRect[new_m_capacity];
		if (!new_rects)
			return false;
		if (m_rects)
			memmove(new_rects, m_rects, sizeof(TBRect) * m_capacity);
		delete [] m_rects;
		m_rects = new_rects;
		m_capacity = new_m_capacity;
	}
	return true;
}

bool TBRegion::AddRect(const TBRect &rect, bool coalesce)
{
	if (coalesce)
	{
		// If the rect can coalesce with any existing rect,
		// just replace it with the union of both, doing coalesce
		// check again recursively.
		for (int i = 0; i < m_num_rects; i++)
		{
			if ( // Can coalesce vertically
				(rect.x == m_rects[i].x && rect.w == m_rects[i].w &&
				(rect.y == m_rects[i].y + m_rects[i].h || rect.y + rect.h == m_rects[i].y))
				|| // Can coalesce horizontally
				(rect.y == m_rects[i].y && rect.h == m_rects[i].h &&
				(rect.x == m_rects[i].x + m_rects[i].w || rect.x + rect.w == m_rects[i].x))
				)
			{
				TBRect union_rect = m_rects[i].Union(rect);
				RemoveRectFast(i);
				return AddRect(union_rect, true);
			}
		}
	}

	if (!GrowIfNeeded())
		return false;
	m_rects[m_num_rects++] = rect;
	return true;
}

bool TBRegion::IncludeRect(const TBRect &include_rect)
{
	for (int i = 0; i < m_num_rects; i++)
	{
		if (include_rect.Intersects(m_rects[i]))
		{
			// Make a region containing the non intersecting parts and then include
			// those recursively (they might still intersect some other part of the region).
			TBRegion inclusion_region;
			if (!inclusion_region.AddExcludingRects(include_rect, m_rects[i], false))
				return false;
			for (int j = 0; j < inclusion_region.m_num_rects; j++)
			{
				if (!IncludeRect(inclusion_region.m_rects[j]))
					return false;
			}
			return true;
		}
	}
	// Now we know that the rect can be added without overlap.
	// Add it with coalesce checking to keep the number of rects down.
	return AddRect(include_rect, true);
}

bool TBRegion::ExcludeRect(const TBRect &exclude_rect)
{
	int num_rects_to_check = m_num_rects;
	for (int i = 0; i < num_rects_to_check; i++)
	{
		if (m_rects[i].Intersects(exclude_rect))
		{
			// Remove the existing rectangle we found we intersect
			// and add the pieces we don't intersect. New rects
			// will be added at the end of the list, so we can decrease
			// num_rects_to_check.
			TBRect rect = m_rects[i];
			RemoveRect(i);
			num_rects_to_check--;
			i--;

			if (!AddExcludingRects(rect, exclude_rect, true))
				return false;
		}
	}
	return true;
}

bool TBRegion::AddExcludingRects(const TBRect &rect, const TBRect &exclude_rect, bool coalesce)
{
	assert(rect.Intersects(exclude_rect));
	TBRect remove = exclude_rect.Clip(rect);

	if (remove.y > rect.y)
		if (!AddRect(TBRect(rect.x, rect.y, rect.w, remove.y - rect.y), coalesce))
			return false;
	if (remove.x > rect.x)
		if (!AddRect(TBRect(rect.x, remove.y, remove.x - rect.x, remove.h), coalesce))
			return false;
	if (remove.x + remove.w < rect.x + rect.w)
		if (!AddRect(TBRect(remove.x + remove.w, remove.y, rect.x + rect.w - (remove.x + remove.w), remove.h), coalesce))
			return false;
	if (remove.y + remove.h < rect.y + rect.h)
		if (!AddRect(TBRect(rect.x, remove.y + remove.h, rect.w, rect.y + rect.h - (remove.y + remove.h)), coalesce))
			return false;
	return true;
}

const TBRect &TBRegion::GetRect(int index) const
{
	assert(index >= 0 && index < m_num_rects);
	return m_rects[index];
}

}; // namespace tinkerbell