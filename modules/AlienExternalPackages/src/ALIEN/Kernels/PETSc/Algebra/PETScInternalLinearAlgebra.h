#ifndef ALIEN_KERNELS_PETSC_ALGEBRA_PETSCINTERNALLINEARALGEBRA_H
#define ALIEN_KERNELS_PETSC_ALGEBRA_PETSCINTERNALLINEARALGEBRA_H

#include <ALIEN/Alien-ExternalPackagesPrecomp.h>

#include <ALIEN/Utils/Precomp.h>

#include <ALIEN/Kernels/PETSc/PETScBackEnd.h>
#include <ALIEN/Core/Backend/IInternalLinearAlgebraT.h>

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Alien {

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

class ALIEN_EXTERNALPACKAGES_EXPORT PETScInternalLinearAlgebra
: public IInternalLinearAlgebra<PETScMatrix, PETScVector>
{
 public:
  PETScInternalLinearAlgebra(Arccore::MessagePassing::IMessagePassingMng* pm = nullptr);
  virtual ~PETScInternalLinearAlgebra();

 public:
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

 private:
  // No member.
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

} // namespace Alien

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#endif /* ALIEN_KERNELS_PETSC_ALGEBRA_PETSCINTERNALLINEARALGEBRA_H */
