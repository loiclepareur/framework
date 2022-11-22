﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2022 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* ArrayExtents.h                                              (C) 2000-2022 */
/*                                                                           */
/* Gestion du nombre d'éléments par dimension pour les tableaux N-dimensions.*/
/*---------------------------------------------------------------------------*/
#ifndef ARCANE_UTILS_ARRAYEXTENTS_H
#define ARCANE_UTILS_ARRAYEXTENTS_H
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arcane/utils/ArrayView.h"
#include "arcane/utils/ArrayBoundsIndex.h"
#include "arcane/utils/ArrayLayout.h"

#include "arccore/base/Span.h"

/*
 * ATTENTION:
 *
 * Toutes les classes de ce fichier sont expérimentales et l'API n'est pas
 * figée. A NE PAS UTILISER EN DEHORS DE ARCANE.
 */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Arcane
{

namespace impl
{
template<int RankValue>
class ArrayExtentsTraits;

template<>
class ArrayExtentsTraits<0>
{
 public:
  static constexpr ARCCORE_HOST_DEVICE std::array<Int32,0>
  extendsInitHelper() { return {}; }
};

template<>
class ArrayExtentsTraits<1>
{
 public:
  static constexpr ARCCORE_HOST_DEVICE std::array<Int32,1>
  extendsInitHelper() { return { 0 }; }
};

template<>
class ArrayExtentsTraits<2>
{
 public:
  static constexpr ARCCORE_HOST_DEVICE std::array<Int32,2>
  extendsInitHelper() { return { 0, 0 }; }
};

template<>
class ArrayExtentsTraits<3>
{
 public:
  static constexpr ARCCORE_HOST_DEVICE std::array<Int32,3>
  extendsInitHelper() { return { 0, 0, 0 }; }
};

template<>
class ArrayExtentsTraits<4>
{
 public:
  static constexpr ARCCORE_HOST_DEVICE std::array<Int32,4>
  extendsInitHelper() { return { 0, 0, 0, 0 }; }
};

}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Spécialisation de ArrayStrideBase pour les tableaux de dimension 0 (les scalaires)
 */
template<>
class ArrayStridesBase<0>
{
 public:
  ArrayStridesBase() = default;
  //! Valeur du pas de la \a i-ème dimension.
  ARCCORE_HOST_DEVICE SmallSpan<const Int32> asSpan() const { return {}; }
  //! Value totale du pas
  ARCCORE_HOST_DEVICE Int64 totalStride() const { return 1; }
  ARCCORE_HOST_DEVICE static ArrayStridesBase<0> fromSpan([[maybe_unused]] Span<const Int32> strides)
  {
    // TODO: vérifier la taille de \a strides
    return {};
  }
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Classe pour conserver le pas dans chaque dimension.
 *
 * Le pas pour une dimension est la distance en mémoire entre deux éléments
 * du tableau pour cette dimension. En général le pas est égal au nombre
 * d'éléments dans la dimension sauf si on utilise des marges (padding) par
 * exemple pour aligner certaines dimensions.
 */
template<int RankValue>
class ArrayStridesBase
{
 public:
  ARCCORE_HOST_DEVICE ArrayStridesBase()
  : m_strides(impl::ArrayExtentsTraits<RankValue>::extendsInitHelper()) { }
  //! Valeur du pas de la \a i-ème dimension.
  ARCCORE_HOST_DEVICE Int32 stride(int i) const { return m_strides[i]; }
  ARCCORE_HOST_DEVICE Int32 operator()(int i) const { return m_strides[i]; }
  ARCCORE_HOST_DEVICE SmallSpan<const Int32> asSpan() const { return { m_strides.data(), RankValue }; }
  //! Valeur totale du pas
  ARCCORE_HOST_DEVICE Int64 totalStride() const
  {
    Int64 nb_element = 1;
    for (int i=0; i<RankValue; i++)
      nb_element *= m_strides[i];
    return nb_element;
  }
  // Instance contenant les dimensions après la première
  ARCCORE_HOST_DEVICE ArrayStridesBase<RankValue-1> removeFirstStride() const
  {
    return ArrayStridesBase<RankValue-1>::fromSpan({m_strides.data()+1,RankValue-1});
  }
  /*!
   * \brief Construit une instance à partir des valeurs données dans \a stride.
   * \pre stride.size() == RankValue.
   */
  ARCCORE_HOST_DEVICE static ArrayStridesBase<RankValue> fromSpan(Span<const Int32> strides)
  {
    ArrayStridesBase<RankValue> v;
    // TODO: vérifier la taille
    for( int i=0; i<RankValue; ++i )
      v.m_strides[i] = strides[i];
    return v;
  }
 protected:
  std::array<Int32,RankValue> m_strides;
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace impl
{
template <class T> constexpr ARCCORE_HOST_DEVICE
T fastmod(T a , T b)
{
  return a < b ? a : a-b*(a/b);
}

template<Int32 Size>
class ExtentValue
{
 public:
  static constexpr Int64 size() { return Size; };
  static constexpr Int32 v = Size;
};

template<>
class ExtentValue<-1>
{
 public:

  constexpr Int64 size() const { return v; }

 public:

  Int32 v = 0;
};
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<Int32... RankSize>
class ArrayExtentsValue;

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<Int32 x0>
class ArrayExtentsValue<x0>
{
 public:

  using IndexType = ArrayBoundsIndex<1>;

  ArrayExtentsValue() = default;

  template<Int32 I> constexpr ARCCORE_HOST_DEVICE Int32 constExtent() const
  {
    static_assert(I==0,"Invalid value for i (i==0)");
    return m_extent0.v;
  }

  constexpr ARCCORE_HOST_DEVICE std::array<Int32,1> asStdArray() const
  {
    return std::array<Int32,1>{m_extent0.v};
  }

  constexpr ARCCORE_HOST_DEVICE Int64 totalNbElement() const
  {
    return m_extent0.v;
  }

  constexpr ARCCORE_HOST_DEVICE IndexType getIndices(Int32 i) const
  {
    return { i };
  }

  constexpr ARCCORE_HOST_DEVICE Int32 extent0() const { return m_extent0.v; };

 protected:

  explicit ARCCORE_HOST_DEVICE ArrayExtentsValue(SmallSpan<const Int32> extents)
  {
    m_extent0.v = extents[0];
  }

  ARCCORE_HOST_DEVICE void _checkIndex([[maybe_unused]] ArrayBoundsIndex<1> idx) const
  {
    ARCCORE_CHECK_AT(idx.id0(),m_extent0.v);
  }

 protected:

  impl::ExtentValue<x0> m_extent0;
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<Int32 x0,Int32 x1>
class ArrayExtentsValue<x0,x1>
{
 public:

  using IndexType = ArrayBoundsIndex<2>;

  ArrayExtentsValue() = default;

  template<Int32 I> constexpr ARCCORE_HOST_DEVICE Int32 constExtent() const
  {
    static_assert(I>=0 && I<2,"Invalid value for I (0<=I<2)");
    if (I==0)
      return m_extent0.v;
    return m_extent1.v;
  }

  constexpr ARCCORE_HOST_DEVICE std::array<Int32,2> asStdArray() const
  {
    return { m_extent0.v, m_extent1.v };
  }

  constexpr ARCCORE_HOST_DEVICE Int64 totalNbElement() const
  {
    return m_extent0.size() * m_extent1.size();
  }

  constexpr ARCCORE_HOST_DEVICE IndexType getIndices(Int32 i) const
  {
    Int32 i1 = impl::fastmod(i,m_extent1.v);
    Int32 i0 = i / m_extent1.v;
    return { i0, i1 };
  }

  constexpr ARCCORE_HOST_DEVICE Int32 extent0() const { return m_extent0.v; };
  constexpr ARCCORE_HOST_DEVICE Int32 extent1() const { return m_extent1.v; };

 protected:

  explicit ARCCORE_HOST_DEVICE ArrayExtentsValue(SmallSpan<const Int32> extents)
  {
    m_extent0.v = extents[0];
    m_extent1.v = extents[1];
  }

  ARCCORE_HOST_DEVICE void _checkIndex([[maybe_unused]] ArrayBoundsIndex<2> idx) const
  {
    ARCCORE_CHECK_AT(idx.id0(),m_extent0.v);
    ARCCORE_CHECK_AT(idx.id1(),m_extent1.v);
  }

 protected:

  impl::ExtentValue<x0> m_extent0;
  impl::ExtentValue<x1> m_extent1;
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<Int32 x0,Int32 x1,Int32 x2>
class ArrayExtentsValue<x0,x1,x2>
{
 public:

  using IndexType = ArrayBoundsIndex<3>;

  ArrayExtentsValue() = default;

  template<Int32 I> constexpr ARCCORE_HOST_DEVICE Int32 constExtent() const
  {
    static_assert(I>=0 && I<3,"Invalid value for I (0<=I<3)");
    if (I==0)
      return m_extent0.v;
    if (I==1)
      return m_extent1.v;
    return m_extent2.v;
  }

  constexpr ARCCORE_HOST_DEVICE std::array<Int32,3> asStdArray() const
  {
    return { m_extent0.v, m_extent1.v, m_extent2.v };
  }

  constexpr ARCCORE_HOST_DEVICE Int64 totalNbElement() const
  {
    return m_extent0.size() * m_extent1.size() * m_extent2.size();
  }

  constexpr ARCCORE_HOST_DEVICE IndexType getIndices(Int32 i) const
  {
    Int32 i2 = impl::fastmod(i,m_extent2.v);
    Int32 fac = m_extent2.v;
    Int32 i1 = impl::fastmod(i / fac,m_extent1.v);
    fac *= m_extent1.v;
    Int32 i0 = i / fac;
    return { i0, i1, i2 };
  }

  constexpr ARCCORE_HOST_DEVICE Int32 extent0() const { return m_extent0.v; };
  constexpr ARCCORE_HOST_DEVICE Int32 extent1() const { return m_extent1.v; };
  constexpr ARCCORE_HOST_DEVICE Int32 extent2() const { return m_extent2.v; };

 protected:

  explicit ARCCORE_HOST_DEVICE ArrayExtentsValue(SmallSpan<const Int32> extents)
  {
    m_extent0.v = extents[0];
    m_extent1.v = extents[1];
    m_extent2.v = extents[2];
  }

  ARCCORE_HOST_DEVICE void _checkIndex([[maybe_unused]] ArrayBoundsIndex<3> idx) const
  {
    ARCCORE_CHECK_AT(idx.id0(),m_extent0.v);
    ARCCORE_CHECK_AT(idx.id1(),m_extent1.v);
    ARCCORE_CHECK_AT(idx.id2(),m_extent2.v);
  }

 protected:

  impl::ExtentValue<x0> m_extent0;
  impl::ExtentValue<x1> m_extent1;
  impl::ExtentValue<x2> m_extent2;
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<Int32 x0,Int32 x1,Int32 x2,Int32 x3>
class ArrayExtentsValue<x0,x1,x2,x3>
{
 public:

  using IndexType = ArrayBoundsIndex<4>;

  ArrayExtentsValue() = default;

  template<Int32 I> constexpr ARCCORE_HOST_DEVICE Int32 constExtent() const
  {
    static_assert(I>=0 && I<4,"Invalid value for I (0<=I<4)");
    if (I==0)
      return m_extent0.v;
    if (I==1)
      return m_extent1.v;
    if (I==2)
      return m_extent2.v;
    return m_extent3.v;
  }

  constexpr ARCCORE_HOST_DEVICE std::array<Int32,4> asStdArray() const
  {
    return { m_extent0.v, m_extent1.v, m_extent2.v, m_extent3.v };
  }

  constexpr ARCCORE_HOST_DEVICE Int64 totalNbElement() const
  {
    return m_extent0.size() * m_extent1.size() * m_extent2.size() * m_extent3.size();
  }

  constexpr ARCCORE_HOST_DEVICE IndexType getIndices(Int32 i) const
  {
    // Compute base indices
    Int32 i3 = impl::fastmod(i,m_extent3.v);
    Int32 fac = m_extent3.v;
    Int32 i2 = impl::fastmod(i/fac,m_extent2.v);
    fac *= m_extent2.v;
    Int32 i1 = impl::fastmod(i/fac,m_extent1.v);
    fac *= m_extent1.v;
    Int32 i0 = i /fac;
    return { i0, i1, i2, i3 };
  }

  constexpr ARCCORE_HOST_DEVICE Int32 extent0() const { return m_extent0.v; };
  constexpr ARCCORE_HOST_DEVICE Int32 extent1() const { return m_extent1.v; };
  constexpr ARCCORE_HOST_DEVICE Int32 extent2() const { return m_extent2.v; };
  constexpr ARCCORE_HOST_DEVICE Int32 extent3() const { return m_extent3.v; };

 protected:

  explicit ARCCORE_HOST_DEVICE ArrayExtentsValue(SmallSpan<const Int32> extents)
  {
    m_extent0.v = extents[0];
    m_extent1.v = extents[1];
    m_extent2.v = extents[2];
    m_extent3.v = extents[3];
  }

  ARCCORE_HOST_DEVICE void _checkIndex([[maybe_unused]] ArrayBoundsIndex<4> idx) const
  {
    ARCCORE_CHECK_AT(idx.id0(),m_extent0.v);
    ARCCORE_CHECK_AT(idx.id1(),m_extent1.v);
    ARCCORE_CHECK_AT(idx.id2(),m_extent2.v);
    ARCCORE_CHECK_AT(idx.id3(),m_extent3.v);
  }

 protected:

  impl::ExtentValue<x0> m_extent0;
  impl::ExtentValue<x1> m_extent1;
  impl::ExtentValue<x2> m_extent2;
  impl::ExtentValue<x3> m_extent3;
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<int RankValue>
class ArrayExtentsValueDynamic;

template<>
class ArrayExtentsValueDynamic<1>
: public ArrayExtentsValue<-1>
{
  using BaseClass = ArrayExtentsValue<-1>;

 public:

  ArrayExtentsValueDynamic() = default;

 protected:

  explicit ARCCORE_HOST_DEVICE ArrayExtentsValueDynamic(SmallSpan<const Int32> extents)
  : BaseClass(extents){}
};

template<>
class ArrayExtentsValueDynamic<2>
: public ArrayExtentsValue<-1,-1>
{
  using BaseClass = ArrayExtentsValue<-1,-1>;

 public:

  ArrayExtentsValueDynamic() = default;

 protected:

  explicit ARCCORE_HOST_DEVICE ArrayExtentsValueDynamic(SmallSpan<const Int32> extents)
  : BaseClass(extents){}
};

template<>
class ArrayExtentsValueDynamic<3>
: public ArrayExtentsValue<-1,-1,-1>
{
  using BaseClass = ArrayExtentsValue<-1,-1,-1>;

 public:

  ArrayExtentsValueDynamic() = default;

 protected:

  explicit ARCCORE_HOST_DEVICE ArrayExtentsValueDynamic(SmallSpan<const Int32> extents)
  : BaseClass(extents){}
};

template<>
class ArrayExtentsValueDynamic<4>
: public ArrayExtentsValue<-1,-1,-1,-1>
{
  using BaseClass = ArrayExtentsValue<-1,-1,-1,-1>;

 public:

  ArrayExtentsValueDynamic() = default;

 protected:

  explicit ARCCORE_HOST_DEVICE ArrayExtentsValueDynamic(SmallSpan<const Int32> extents)
  : BaseClass(extents){}
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Spécialisation de ArrayExtentsBase pour les tableaux de dimension 0 (les scalaires)
 */
template<>
class ArrayExtentsBase<MDDim0>
{
 public:
  ArrayExtentsBase() = default;
  //! Nombre d'élément de la \a i-ème dimension.
  constexpr ARCCORE_HOST_DEVICE SmallSpan<const Int32> asSpan() const { return {}; }
  //! Nombre total d'eléments
  constexpr ARCCORE_HOST_DEVICE Int32 totalNbElement() const { return 1; }
  ARCCORE_HOST_DEVICE static ArrayExtentsBase<MDDim0> fromSpan([[maybe_unused]] Span<const Int32> extents)
  {
    // TODO: vérifier la taille de \a extents
    return {};
  }
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Classe pour conserver le nombre d'éléments dans chaque dimension.
 */
template<typename ExtentType>
class ArrayExtentsBase
: public ArrayExtentsValueDynamic<ExtentType::rank()>
{
  using BaseClass = ArrayExtentsValueDynamic<ExtentType::rank()>;
  using ArrayExtentsPreviousRank = ArrayExtentsBase<MDDim<ExtentType::rank()-1>>;

 public:

  using BaseClass::totalNbElement;
  using BaseClass::getIndices;

 public:

  ARCCORE_HOST_DEVICE constexpr ArrayExtentsBase()
  : BaseClass(), m_extents(impl::ArrayExtentsTraits<ExtentType::rank()>::extendsInitHelper()) { }

 protected:

  explicit ARCCORE_HOST_DEVICE ArrayExtentsBase(SmallSpan<const Int32> extents)
  : BaseClass(extents)
  {
    auto nb_rank = ExtentType::rank();
    Integer n = extents.size();
    Integer vn = math::min(n,nb_rank);
    for( int i=0; i<vn; ++i )
      m_extents[i] = extents[i];
    for( int i=vn; i<nb_rank; ++i )
      m_extents[i] = 0;
  }

 public:

  //! TEMPORARY: Positionne à \a v le nombre d'éléments de la dimension 0.
  ARCCORE_HOST_DEVICE void setExtent0(Int32 v) { m_extents[0] = v; this->m_extent0.v = v; }

  // Instance contenant les dimensions après la première
  ARCCORE_HOST_DEVICE ArrayExtentsPreviousRank removeFirstExtent() const
  {
    return ArrayExtentsPreviousRank::fromSpan({m_extents.data()+1,ExtentType::rank()-1});
  }
  /*!
   * \brief Construit une instance à partir des valeurs données dans \a extents.
   */
  ARCCORE_HOST_DEVICE static ArrayExtentsBase<ExtentType> fromSpan(SmallSpan<const Int32> extents)
  {
    return ArrayExtentsBase<ExtentType>(extents);
  }

 protected:

  std::array<Int32,ExtentType::rank()> m_extents;
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<>
class ArrayExtents<MDDim1>
: public ArrayExtentsBase<MDDim1>
{
 public:

  using BaseClass = ArrayExtentsBase<MDDim1>;
  using BaseClass::totalNbElement;

 public:

  ArrayExtents() = default;
  constexpr ARCCORE_HOST_DEVICE ArrayExtents(const BaseClass&rhs) : BaseClass(rhs) {}
  constexpr ARCCORE_HOST_DEVICE explicit ArrayExtents(Int32 dim1_size)
  {
    m_extents[0] = dim1_size;

    this->m_extent0.v = dim1_size;
  }
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<>
class ArrayExtents<MDDim2>
: public ArrayExtentsBase<MDDim2>
{
 public:

  using BaseClass = ArrayExtentsBase<MDDim2>;
  using BaseClass::totalNbElement;

 public:

  ArrayExtents() = default;
  constexpr ARCCORE_HOST_DEVICE ArrayExtents(const BaseClass& rhs) : BaseClass(rhs){}
  constexpr ARCCORE_HOST_DEVICE ArrayExtents(Int32 dim1_size,Int32 dim2_size)
  {
    m_extents[0] = dim1_size;
    m_extents[1] = dim2_size;

    this->m_extent0.v = dim1_size;
    this->m_extent1.v = dim2_size;
  }
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<>
class ArrayExtents<MDDim3>
: public ArrayExtentsBase<MDDim3>
{
 public:

  using BaseClass = ArrayExtentsBase<MDDim3>;
  using BaseClass::totalNbElement;

 public:

  ArrayExtents() = default;
  constexpr ARCCORE_HOST_DEVICE ArrayExtents(const BaseClass& rhs) : BaseClass(rhs){}
  constexpr ARCCORE_HOST_DEVICE ArrayExtents(Int32 dim1_size,Int32 dim2_size,Int32 dim3_size)
  {
    m_extents[0] = dim1_size;
    m_extents[1] = dim2_size;
    m_extents[2] = dim3_size;

    this->m_extent0.v = dim1_size;
    this->m_extent1.v = dim2_size;
    this->m_extent2.v = dim3_size;
  }
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<>
class ArrayExtents<MDDim4>
: public ArrayExtentsBase<MDDim4>
{
 public:
  using BaseClass = ArrayExtentsBase<MDDim4>;
  using BaseClass::totalNbElement;
 public:
  ArrayExtents() = default;
  constexpr ARCCORE_HOST_DEVICE ArrayExtents(const BaseClass& rhs) : BaseClass(rhs){}
  constexpr ARCCORE_HOST_DEVICE ArrayExtents(Int32 dim1_size,Int32 dim2_size,Int32 dim3_size,Int32 dim4_size)
  {
    m_extents[0] = dim1_size;
    m_extents[1] = dim2_size;
    m_extents[2] = dim3_size;
    m_extents[3] = dim4_size;

    this->m_extent0.v = dim1_size;
    this->m_extent1.v = dim2_size;
    this->m_extent2.v = dim3_size;
    this->m_extent3.v = dim4_size;
  }
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<typename LayoutType>
class ArrayExtentsWithOffset<MDDim1,LayoutType>
: private ArrayExtents<MDDim1>
{
 public:
  using BaseClass = ArrayExtents<MDDim1>;
  using BaseClass::extent0;
  using BaseClass::asStdArray;
  using BaseClass::totalNbElement;
  using BaseClass::getIndices;
  using Layout = LayoutType;
 public:
  ArrayExtentsWithOffset() = default;
  ARCCORE_HOST_DEVICE ArrayExtentsWithOffset(const ArrayExtents<MDDim1>& rhs)
  : BaseClass(rhs)
  {
  }
  constexpr ARCCORE_HOST_DEVICE Int64 offset(Int32 i) const
  {
    BaseClass::_checkIndex(i);
    return i;
  }
  constexpr ARCCORE_HOST_DEVICE Int64 offset(ArrayBoundsIndex<1> idx) const
  {
    BaseClass::_checkIndex(idx.id0());
    return idx.id0();
  }
  constexpr BaseClass extents() const { const BaseClass* b = this; return *b; }
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<typename LayoutType>
class ArrayExtentsWithOffset<MDDim2,LayoutType>
: private ArrayExtents<MDDim2>
{
 public:
  using BaseClass = ArrayExtents<MDDim2>;
  using BaseClass::extent0;
  using BaseClass::extent1;
  using BaseClass::asStdArray;
  using BaseClass::totalNbElement;
  using BaseClass::getIndices;
  using Layout = LayoutType;
 public:
  ArrayExtentsWithOffset() = default;
  constexpr ARCCORE_HOST_DEVICE ArrayExtentsWithOffset(ArrayExtents<MDDim2> rhs)
  : BaseClass(rhs)
  {
  }
  constexpr ARCCORE_HOST_DEVICE Int64 offset(Int32 i,Int32 j) const
  {
    return offset({i,j});
  }
  constexpr ARCCORE_HOST_DEVICE Int64 offset(ArrayBoundsIndex<2> idx) const
  {
    BaseClass::_checkIndex(idx);
    return Layout::offset(idx,this->constExtent<Layout::LastExtent>());
  }
  constexpr BaseClass extents() const { const BaseClass* b = this; return *b; }
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<typename LayoutType>
class ArrayExtentsWithOffset<MDDim3,LayoutType>
: private ArrayExtents<MDDim3>
{
 public:
  using BaseClass = ArrayExtents<MDDim3>;
  using BaseClass::extent0;
  using BaseClass::extent1;
  using BaseClass::extent2;
  using BaseClass::asStdArray;
  using BaseClass::totalNbElement;
  using BaseClass::getIndices;
  using Layout = LayoutType;
 public:
  ArrayExtentsWithOffset() = default;
  constexpr ARCCORE_HOST_DEVICE ArrayExtentsWithOffset(ArrayExtents<MDDim3> rhs)
  : BaseClass(rhs)
  {
    _computeOffsets();
  }
  constexpr ARCCORE_HOST_DEVICE Int64 offset(Int32 i,Int32 j,Int32 k) const
  {
    return offset({i,j,k});
  }
  constexpr ARCCORE_HOST_DEVICE Int64 offset(ArrayBoundsIndex<3> idx) const
  {
    this->_checkIndex(idx);
    return Layout::offset(idx,this->constExtent<Layout::LastExtent>(),m_dim23_size);
  }
  constexpr BaseClass extents() const { const BaseClass* b = this; return *b; }
 protected:
  ARCCORE_HOST_DEVICE void _computeOffsets()
  {
    m_dim23_size = Layout::computeOffsetIndexes(m_extents);
  }
 private:
  Int64 m_dim23_size = 0;
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<typename LayoutType>
class ArrayExtentsWithOffset<MDDim4,LayoutType>
: private ArrayExtents<MDDim4>
{
 public:
  using BaseClass = ArrayExtents<MDDim4>;
  using BaseClass::extent0;
  using BaseClass::extent1;
  using BaseClass::extent2;
  using BaseClass::extent3;
  using BaseClass::asStdArray;
  using BaseClass::totalNbElement;
  using BaseClass::getIndices;
  using Layout = LayoutType;
 public:
  ArrayExtentsWithOffset() = default;
  constexpr ARCCORE_HOST_DEVICE ArrayExtentsWithOffset(ArrayExtents<MDDim4> rhs)
  : BaseClass(rhs)
  {
    _computeOffsets();
  }
  constexpr ARCCORE_HOST_DEVICE Int64 offset(Int32 i,Int32 j,Int32 k,Int32 l) const
  {
    return offset({i,j,k,l});
  }
  constexpr ARCCORE_HOST_DEVICE Int64 offset(ArrayBoundsIndex<4> idx) const
  {
    this->_checkIndex(idx);
    return (m_dim234_size*idx.largeId0()) + m_dim34_size*idx.largeId1() + this->m_extent3.v*idx.largeId2() + idx.largeId3();
  }
  BaseClass extents() const { const BaseClass* b = this; return *b; }
 protected:
  ARCCORE_HOST_DEVICE void _computeOffsets()
  {
    m_dim34_size = Int64(this->m_extent2.v) * Int64(this->m_extent3.v);
    m_dim234_size = Int64(m_dim34_size) * Int64(this->m_extent1.v);
  }
 private:
  Int64 m_dim34_size = 0; //!< dim3 * dim4
  Int64 m_dim234_size = 0; //!< dim2 * dim3 * dim4
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

} // End namespace Arcane

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#endif  
