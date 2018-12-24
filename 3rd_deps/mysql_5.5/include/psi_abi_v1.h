/*
   Copyright (c) 2008, 2010, Oracle and/or its affiliates. All rights reserved.










   The lines above are intentionally left blank
*/

/**
  @file mysql/psi/psi_abi_v1.h
  ABI check for mysql/psi/psi.h, when using PSI_VERSION_1.
  This file is only used to automate detection of changes between versions.
  Do not include this file, include mysql/psi/psi.h instead.
*/
#define USE_PSI_1
#define HAVE_PSI_INTERFACE
#define _global_h
#include "mysql/psi/psi.h"

