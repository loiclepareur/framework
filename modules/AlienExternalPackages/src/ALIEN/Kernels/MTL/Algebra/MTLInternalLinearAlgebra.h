// -*- C++ -*-
#ifndef ALIEN_KERNELS_MTL_ALGEBRA_MTLINTERNALLINEARALGEBRA_H
#define ALIEN_KERNELS_MTL_ALGEBRA_MTLINTERNALLINEARALGEBRA_H

#include <ALIEN/Alien-ExternalPackagesPrecomp.h>

#include <ALIEN/Utils/Precomp.h>

#include <ALIEN/Kernels/MTL/MTLBackEnd.h>
#include <ALIEN/Core/Backend/IInternalLinearAlgebraT.h>

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Alien {

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

class ALIEN_EXTERNALPACKAGES_EXPORT MTLInternalLinearAlgebra
: public IInternalLinearAlgebra<MTLMatrix, MTLVector>
{
 public:
  MTLInternalLinearAlgebra();
  virtual ~MTLInternalLinearAlgebra();

  // IInternalLinearAlgebra interface.
  Arccore::Real norm0(const Vector& x) const;
  Arccore::Real norm1(const Vector& x) const;
  Arccore::Real norm2(const Vector& x) const;
  void mult(const Matrix& a, const Vector& x, Vector& r) const;
  void axpy(const Arccore::Real& alpha, const Vector& x, Vector& r) const;
  void aypx(const Arccore::Real& alpha, Vector& y, const Vector& x) const;
  void copy(const Vector& x, Vector& r) const;
  Arccore::Real dot(const Vector& x, const Vector& y) const;
  void scal(const Arccore::Real& alpha, Vector& x) const;
  void diagonal(const Matrix& a, Vector& x) const;
  void reciprocal(Vector& x) const;
  void pointwiseMult(const Vector& x, const Vector& y, Vector& w) const;

  void mult(const Matrix& a, const UniqueArray<Real>& x, UniqueArray<Real>& r) const ;
  void axpy(const Real & alpha, const UniqueArray<Real>& x, UniqueArray<Real>& r) const;
  void aypx(const Real & alpha, UniqueArray<Real>& y, const UniqueArray<Real>& x) const;
  void copy(const UniqueArray<Real>& x, UniqueArray<Real>& r) const;
  Real dot(Integer local_size, const UniqueArray<Real>& x, const UniqueArray<Real>& y) const;
  void scal(const Real & alpha, UniqueArray<Real>& x) const;
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

} // namespace Alien

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#endif /* ALIEN_KERNELS_MTL_ALGEBRA_MTLINTERNALLINEARALGEBRA_H */
