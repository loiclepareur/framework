// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2022 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* PDESRandomNumberGeneratorService.cc                         (C) 2000-2022 */
/*                                                                           */
/* Implémentation d'un générateur de nombres aléatoires LCG.                 */
/* Inspiré du générateur de Quicksilver (LLNL) et des pages 302-304          */
/* du livre :                                                                */
/*                                                                           */
/*   Numerical Recipes in C                                                  */
/*   The Art of Scientific Computing                                         */
/*   Second Edition                                                          */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arcane/IRandomNumberGenerator.h"
#include "arcane/std/PDESRandomNumberGenerator_axl.h"

using namespace Arcane;

class PDESRandomNumberGeneratorService
: public ArcanePDESRandomNumberGeneratorObject
{
 public:
  PDESRandomNumberGeneratorService(const ServiceBuildInfo& sbi)
  : ArcanePDESRandomNumberGeneratorObject(sbi)
  , m_seed(0)
  {
    m_with_option = (sbi.creationType() == ST_CaseOption);
  }

  virtual ~PDESRandomNumberGeneratorService(){};

 public:
  void initSeed() override;
  void initSeed(Int64 seed) override;

  Int64 seed() override;

  bool isLeapSeedSupported() override {return true;};
  Int64 generateRandomSeed(Integer leap) override;
  Int64 generateRandomSeed(Int64* parent_seed, Integer leap) override;

  bool isLeapNumberSupported() override { return true; };
  Real generateRandomNumber(Integer leap) override;
  Real generateRandomNumber(Int64* seed, Integer leap) override;

 protected:
  void _breakupUInt64(uint64_t uint64_in, uint32_t* front_bits, uint32_t* back_bits);
  uint64_t _reconstructUInt64(uint32_t front_bits, uint32_t back_bits);
  void _psdes(uint32_t* lword, uint32_t* irword);
  uint64_t _hashState(uint64_t initial_number);
  Real _ran4(Int64* seed, Integer leap);

 protected:
  Int64 m_seed;
  bool m_with_option;
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ARCANE_REGISTER_SERVICE_PDESRANDOMNUMBERGENERATOR(PDESRandomNumberGenerator, PDESRandomNumberGeneratorService);

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
