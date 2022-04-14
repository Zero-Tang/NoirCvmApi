/*
  NoirCvmApi - NoirVisor Customizable VM API Library

  Copyright 2021-2022, Zero Tang. All rights reserved.

  This file defines the status indicator in NoirCvmApi.

  This program is distributed in the hope that it will be useful, but
  without any warranty (no matter implied warranty or merchantability
  or fitness for a particular purpose, etc.).
*/

/*
  Introduction to NV-Status Layout.
  NV-Status is a 32-bit value and consists of following parts:

  S - Severity Code (2-bits, bits 30-31)
	00 - Success
	01 - Info
	02 - Warning
	03 - Error

  F - Facility Code (6-bits, bits 24-29)
	00 - Customizable VM
	01 - Specific to Intel VT-x
	02 - Specific to AMD-V
	Other values (03-63) are reserved for future.

  C - Detailed Code (24-bits, bits 0-23)

  To visualize the NV-Status, we have the following layout graph:
  31	30			24												0
  +-----+-----------+-----------------------------------------------+
  |  S  |	  F		| Detailed Code									|
  +-----+-----------+-----------------------------------------------+
*/

#pragma once

// Macros for status.
#define NOIR_STATUS_SEVERITY(st)		((st>>30)&0x03)
#define NOIR_STATUS_FACILITY(st)		((st>>24)&0x3F)
#define NOIR_STATUS_CODE(st)			(st&0xFFFFFF)

/*
  Status Indicator: NOIR_SUCCESS

  If a procedure is executed successfully,
  this value is supposed to be returned.
*/

#define NOIR_SUCCESS					0x0

/*
  Status Indicator: NOIR_ALREADY_RESCINDED
  
  If execution of a vCPU is already rescinded,
  this value is supposed to be returned.
*/

#define NOIR_ALREADY_RESCINDED			0x40000001

/*
  Status Indicator: NOIR_UNSUCCESSFUL

  If a procedure failed to execute due to unknown
  error, this value is supposed to be returned.
*/

#define NOIR_UNSUCCESSFUL				0xC0000000

/*
  Status Indicator: NOIR_INSUFFICIENT_RESOURCES

  If a procedure encounters lack of resource,
  this value is supposed to be returned.
*/

#define NOIR_INSUFFICIENT_RESOURCES		0xC0000001

/*
  Status Indicator: NOIR_NOT_IMPLEMENTED

  If a function is not yet implemented,
  this value is supposed to be returned.
*/

#define NOIR_NOT_IMPLEMENTED			0xC0000002

/*
  Status Indicator: NOIR_UNKNOWN_PROCESSOR

  If NoirVisor detected an unknown processor,
  this value is supposed to be returned.
*/

#define NOIR_UNKNOWN_PROCESSOR			0xC0000003

/*
  Status Indicator: NOIR_INVALID_PARAMTER

  If invalid parameters are passed to this function,
  then this value is supposed to be returned.
*/

#define NOIR_INVALID_PARAMETER			0xC0000004

/*
  Status Indicator: NOIR_HYPERVISION_ABSENT

  If the function requires hypervisor to be present
  in the system and hypervisor is actually absent,
  then this value is supposed to be returned.
*/

#define NOIR_HYPERVISION_ABSENT			0xC0000005

/*
  Status Indicator: NOIR_VCPU_ALREADY_CREATED

  If an attempt to create a vCPU with identifier
  that is already created within a Customizable VM,
  then this value is supposed to be returned.
*/

#define NOIR_VCPU_ALREADY_CREATED		0xC0000006

/*
  Status Indicator: NOIR_BUFFER_TOO_SMALL

  If the buffer passed to the function is too small,
  then this value is supposed to be returned.
*/

#define NOIR_BUFFER_TOO_SMALL			0xC0000007

/*
  Status Indicator: NOIR_VCPU_NOT_EXIST

  If a specified vCPU does not exist, then
  this value is supposed to be returned.
*/

#define NOIR_VCPU_NOT_EXIST				0xC0000008

/*
  Status Indicator: NOIR_USER_PAGE_VIOLATION

  If the page specified for CVM address
  mapping does not meet the requirements,
  then this value is supposed to be returned.
*/

#define NOIR_USER_PAGE_VIOLATION		0xC0000009

/*
  Status Indicator: NOIR_GUEST_PAGE_ABSENT

  If a guest page to be accessed by
  the hypervisor does not exist,
  then this value is supposed to be returned.
*/

#define NOIR_GUEST_PAGE_ABSENT			0xC000000A