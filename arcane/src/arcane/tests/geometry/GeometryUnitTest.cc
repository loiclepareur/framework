﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2022 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------

#include "arcane/BasicUnitTest.h"

#include "arcane/tests/ArcaneTestGlobal.h"
#include "arcane/geometry/IGeometry.h"
#include "arcane/geometry/IGeometryMng.h"

using namespace Arcane::Numerics;
#include "arcane/tests/geometry/GeometryUnitTest_axl.h"

#include "arcane/IItemOperationByBasicType.h"
#include "arcane/IParallelMng.h"
#include "arcane/MeshStats.h"
#include "arcane/mesh/DynamicMesh.h"

#include "arcane/IMesh.h"
#include "arcane/IMeshSubMeshTransition.h"
#include "arcane/IItemFamily.h"

#include <arcane/utils/Collection.h>
#include <arcane/IItemFamily.h>
#include <arcane/ArcaneTypes.h>

#include <arcane/utils/PlatformUtils.h>
#include <exception>
#include "arcane/geometry/ItemGroupBuilder.h"

#include <arcane/ITimeLoopMng.h>
#include <arcane/IParallelMng.h>
#include <arcane/utils/Limits.h>

#include <stdlib.h> // pour *rand48

/* Petite note indicative
 * Arcane a quelques problèmes avec les variables partielles
 * - Si m_cell_group and utilise des RealVariable => pb (dans volumeCompute)
 * - Si groupe isOwn possible problème d'enregistrement interne (Arcane) de
 *   de la variable (avec version >= 1.4.0, semble etre de meme)
 */

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace ArcaneTest
{


using namespace Arcane;

class GeometryUnitTest
: public ArcaneGeometryUnitTestObject
{
 public:
  /** Constructeur de la classe */
  GeometryUnitTest(const ServiceBuildInfo& cb)
    : ArcaneGeometryUnitTestObject(cb)
  {
    ;
  }

  /** Destructeur de la classe */
  virtual ~GeometryUnitTest() {}
  
 public:
  virtual void initializeTest();
  virtual void executeTest();

  /** Retourne le numéro de version du module */
  virtual Arcane::VersionInfo versionInfo() const { return Arcane::VersionInfo(1,0,0); }
  class Toto
  {
  public :
    Toto()
    {
      cout<<"New Toto" ;
    }
    ~Toto()
    {
      cout<<"Delete Toto" ;
    }
  };
private:
  CellGroup m_cell_group;
  NodeGroup m_border_node_group;
  FaceGroup m_border_face_group;

private:
  Real volumeCompute();
  Real surfaceCompute();
  IGeometryMng* m_geom_service = nullptr;
  IGeometry* m_geom = nullptr;
};

void GeometryUnitTest::
initializeTest()
{
  //m_cell_group = ownCells();
  m_cell_group = allCells();

  ItemGroupBuilder<Node> nodeBuilder(subDomain()->defaultMesh(),"BorderNodes");
  ItemGroupBuilder<Face> faceBuilder(subDomain()->defaultMesh(),"BorderFaces");
  
  ENUMERATE_FACE(iface,ownFaces()) {
    const Face face = *iface;
    if (face.isSubDomainBoundary()) {
      nodeBuilder.add(face.nodes());
      faceBuilder.add(face);

      // Check isBoundaryOutside
      if (face.isSubDomainBoundaryOutside() && face.backCell().null())
        ARCANE_FATAL("Test failed : Bad face boundary");
    }
  }

  m_border_node_group = nodeBuilder.buildGroup();
  m_border_face_group = mesh()->outerFaces();

  FaceGroup border_face_group_test = faceBuilder.buildGroup();
  info() << "Border Node Number = " << m_border_node_group.size();
  info() << "Border Face Number = " << m_border_face_group.size();
  if (m_border_face_group.own().size() != border_face_group_test.size())
    ARCANE_FATAL("Test failed : Bad OuterFaces group s1={0} s2={1}",
                 m_border_face_group.own().size(),border_face_group_test.size());

  //// 
  m_geom_service = options()->geometry();
  m_geom_service->init();
  m_geom = m_geom_service->geometry() ;
  m_geom_service->addItemGroupProperty(m_cell_group, IGeometryProperty::PVolume,m_volumes);
  m_geom_service->addItemGroupProperty(m_border_face_group, IGeometryProperty::PArea,m_surfaces);

  m_geom_service->update() ;
  Real area = surfaceCompute();
  area = subDomain()->parallelMng()->reduce(Parallel::ReduceSum,area);
  Real volume = volumeCompute();
  volume = subDomain()->parallelMng()->reduce(Parallel::ReduceSum,volume);

  pinfo() << "Initial Total measure : volume = " << volume << ", area = " << area;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

static double _myrandom()
{
#ifdef ARCANE_OS_WIN32
  return (double)( rand() / (RAND_MAX + 1) );
#else
  return drand48();
#endif
}

void 
GeometryUnitTest::
executeTest()
{
  const Real factor = options()->factor();
  const Real twopi = 2. * 3.141592653589;

  VariableNodeReal3 & coords = PRIMARYMESH_CAST(mesh())->nodesCoordinates();
  ENUMERATE_NODE(inode,ownNodes()) {
    const Real r = _myrandom()*factor;
    const Real theta = twopi*_myrandom();
    const Real phi = twopi*_myrandom();
    coords[inode] += r*Real3(cos(theta),sin(theta)*cos(phi),sin(theta)*sin(phi));
  }
  
  coords.synchronize();
  m_geom_service->update() ;
  Real area = surfaceCompute();
  area = subDomain()->parallelMng()->reduce(Parallel::ReduceSum,area);
  Real volume = volumeCompute();
  volume = subDomain()->parallelMng()->reduce(Parallel::ReduceSum,volume);
  pinfo() << "Total measure : volume = " << volume << ", area = " << area;

  if (globalIteration() >= options()->niter())
    subDomain()->timeLoopMng()->stopComputeLoop(true);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

Real
GeometryUnitTest::
volumeCompute()
{
  const IGeometryMng::RealVariable & measures = m_geom_service->getRealVariableProperty(m_cell_group,IGeometryProperty::PVolume);
  //const IGeometryMng::RealGroupMap & measures = m_geom_service->getRealGroupMapProperty(m_cell_group,IGeometryProperty::PVolume);
  Real volume = 0;
  Real maxlVolume = -FloatInfo<Real>::maxValue();
  Real minlVolume = +FloatInfo<Real>::maxValue();
  ENUMERATE_CELL(icell,ownCells()) {
    const Real lVolume = measures[*icell];
    if (!math::isNearlyZero(lVolume - m_geom->computeMeasure(*icell)))
      ARCANE_FATAL("Conflicting Volume measure : {0} vs {1} diff={2}",
                   lVolume, m_geom->computeMeasure(*icell),lVolume - m_geom->computeMeasure(*icell));
    minlVolume = math::min(minlVolume,lVolume);
    maxlVolume = math::max(maxlVolume,lVolume);
    volume += lVolume;
  }

  if (minlVolume < maxlVolume)
    info() << "Local volume amplitude : " << minlVolume << " -> " << maxlVolume;
  return volume;
}

Real
GeometryUnitTest::
surfaceCompute()
{
  const IGeometryMng::RealVariable & measures = m_geom_service->getRealVariableProperty(m_border_face_group,IGeometryProperty::PArea);
  //const IGeometryMng::RealGroupMap & measures = m_geom_service->getRealGroupMapProperty(m_border_face_group,IGeometryProperty::PArea);
  Real area = 0;
  Real maxlArea = -FloatInfo<Real>::maxValue();
  Real minlArea = +FloatInfo<Real>::maxValue();
  ENUMERATE_FACE(iface,m_border_face_group) {
    const Real lArea = measures[*iface];
    if (!math::isNearlyZero(lArea - m_geom->computeArea(*iface)))
      ARCANE_FATAL("Conflicting Area measure : {0} vs {1} diff={2}",
                   lArea, m_geom->computeArea(*iface), lArea - m_geom->computeArea(*iface));
    minlArea = math::min(minlArea,lArea);
    maxlArea = math::max(maxlArea,lArea);
    area += lArea;
  }

  if (minlArea < maxlArea)
    info() << "Local area amplitude : " << minlArea << " -> " << maxlArea;
  return area;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ARCANE_REGISTER_SERVICE_GEOMETRYUNITTEST(GeometryUnitTest,GeometryUnitTest);


} // End namespace ArcaneTest

