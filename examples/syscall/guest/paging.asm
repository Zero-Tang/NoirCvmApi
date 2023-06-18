; Paging Structure of Guest Program
bits 64
org 0x0

pml4e_base:
	dq 0x1007		; Point to PDPTE
	times 511 dq 0
pdpte_base:
	dq 0x2007		; Point to PDE
	times 511 dq 0
pde_base:
	dq 0x87         ; 2MiB Large Page
	times 511 dq 0