#pragma once

#define PAGE_SIZE			0x1000
#define PAGE_OFFSET(x)		(x&0xfff)
#define PAGE_BASE(x)		(x&0xfffffffffffff000)
#define NEXT_PAGE_BASE(x)	(PAGE_BASE(x)+PAGE_SIZE)

#define PAGE_OVERFLOW(x,s)	((x+s)>NEXT_PAGE_BASE(x))