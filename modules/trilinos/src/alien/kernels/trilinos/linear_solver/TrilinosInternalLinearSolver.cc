
/* Author : desrozis at Mon Mar 30 15:06:37 2009
 * Generated by createNew
 */
#define MPICH_SKIP_MPICXX 1
#include "mpi.h"

#include <vector>

#include "alien/kernels/trilinos/TrilinosPrecomp.h"
#include <Kokkos_Macros.hpp>
#include <KokkosCompat_ClassicNodeAPI_Wrapper.hpp>

#include <alien/data/Space.h>
#include <alien/expression/solver/ILinearSolver.h>
#include <alien/expression/solver/ILinearAlgebra.h>
#include <alien/expression/solver/solver_stats/SolverStat.h>
#include <alien/expression/solver/solver_stats/SolverStater.h>

#include <alien/kernels/simple_csr/SimpleCSRPrecomp.h>
#include <alien/kernels/simple_csr/algebra/SimpleCSRLinearAlgebra.h>

#include <alien/core/impl/MultiMatrixImpl.h>
#include <alien/core/impl/MultiVectorImpl.h>

#include <alien/AlienRefSemantic.h>

#include <alien/kernels/trilinos/TrilinosBackEnd.h>
#include <alien/kernels/trilinos/data_structure/TrilinosInternal.h>
#include <alien/kernels/trilinos/algebra/TrilinosLinearAlgebra.h>

#include <alien/kernels/trilinos/data_structure/TrilinosMatrix.h>
#include <alien/kernels/trilinos/data_structure/TrilinosVector.h>

#include <alien/kernels/trilinos/linear_solver/TrilinosOptionTypes.h>
#include <alien/AlienTrilinosPrecomp.h>

#include <ALIEN/axl/TrilinosSolver_IOptions.h>
#include <alien/kernels/trilinos/linear_solver/TrilinosInternalSolver.h>
#include <alien/kernels/trilinos/linear_solver/TrilinosInternalLinearSolver.h>
#include <alien/core/backend/LinearSolverT.h>
#include <alien/core/block/ComputeBlockOffsets.h>
#include <arccore/message_passing_mpi/MpiMessagePassingMng.h>

/*---------------------------------------------------------------------------*/
const std::string TrilinosOptionTypes::solver_type[NumOfSolver] = { "BICGSTAB", "CG",
  "GMRES", "ML", "MueLU", "KLU2" };

const std::string TrilinosOptionTypes::preconditioner_type[NumOfPrecond] = {
  "None", "RELAXATION", "CHEBYSHEV", "RILUK", "ILUT", "FAST_ILU", "SCHWARZ", "ML",
  "MueLu",
};

namespace Alien {

template class ALIEN_TRILINOS_EXPORT LinearSolver<BackEnd::tag::tpetraserial>;
#ifdef KOKKOS_ENABLE_OPENMP
template class ALIEN_TRILINOS_EXPORT LinearSolver<BackEnd::tag::tpetraomp>;
#endif
#ifdef KOKKOS_ENABLE_THREADS
template class ALIEN_TRILINOS_EXPORT LinearSolver<BackEnd::tag::tpetrapth>;
#endif
#ifdef KOKKOS_ENABLE_CUDA
template class ALIEN_TRILINOS_EXPORT LinearSolver<BackEnd::tag::tpetracuda>;
#endif

/*---------------------------------------------------------------------------*/
template <typename TagT>
TrilinosInternalLinearSolver<TagT>::TrilinosInternalLinearSolver(
    Arccore::MessagePassing::IMessagePassingMng* parallel_mng,
    IOptionsTrilinosSolver* options)
: m_parallel_mng(parallel_mng)
, m_options(options)
{
}

template <typename TagT>
void
TrilinosInternalLinearSolver<TagT>::init(int argc, char const** argv)
{
}

template <typename TagT>
String
TrilinosInternalLinearSolver<TagT>::getBackEndName() const
{
  return AlgebraTraits<TagT>::name();
}

/*---------------------------------------------------------------------------*/
template <typename TagT>
void
TrilinosInternalLinearSolver<TagT>::init()
{
  bool use_amgx = false;
  if (m_options->muelu().size() > 0 && m_options->muelu()[0]->amgx().size() > 0
      && m_options->muelu()[0]->amgx()[0]->enable())
    use_amgx = true;
  TrilinosInternal::TrilinosInternal::initialize(m_parallel_mng,
      TrilinosInternal::TrilinosInternal::Node<TagT>::execution_space_name,
      m_options->nbThreads(), use_amgx);
  assert(TrilinosInternal::TrilinosInternal::Node<TagT>::execution_space_name.compare(
             TrilinosInternal::TrilinosInternal::getExecutionSpace())
      == 0);

  traceMng()->info() << "TRILINOS EXECUTION SPACE : "
                     << TrilinosInternal::TrilinosInternal::getExecutionSpace();
  traceMng()->info() << "TRILINOS NB THREADS      : "
                     << TrilinosInternal::TrilinosInternal::getNbThreads();
  m_output_level = m_options->output();

  m_trilinos_solver.reset(new TrilinosInternal::SolverInternal<TagT>());
  m_precond_name = TrilinosOptionTypes::precondName(m_options->preconditioner());
  auto* mpi_mng =
      dynamic_cast<Arccore::MessagePassing::Mpi::MpiMessagePassingMng*>(m_parallel_mng);
  const MPI_Comm* comm = static_cast<const MPI_Comm*>(mpi_mng->getMPIComm());
  m_trilinos_solver->initPrecondParameters(m_options, comm);

  m_solver_name = TrilinosOptionTypes::solverName(m_options->solver());
  m_trilinos_solver->initSolverParameters(m_options);
}
template <typename TagT>
void
TrilinosInternalLinearSolver<TagT>::setMatrixCoordinate(
    Matrix const& A, Vector const& x, Vector const& y, Vector const& z)
{

  const CSRMatrixType& matrix = A.impl()->template get<TagT>();

  typedef typename solver_type::vec_type VectorType;
  m_trilinos_solver->m_coordinates =
      rcp(new VectorType(matrix.internal()->m_map, 3, false));

  Teuchos::ArrayRCP<Teuchos::ArrayRCP<typename solver_type::real_type>> Coord(3);
  Coord[0] = m_trilinos_solver->m_coordinates->getDataNonConst(0);
  Coord[1] = m_trilinos_solver->m_coordinates->getDataNonConst(1);
  Coord[2] = m_trilinos_solver->m_coordinates->getDataNonConst(2);

  Alien::VectorReader x_view(x);
  Alien::VectorReader y_view(y);
  Alien::VectorReader z_view(z);

  for (int i = 0; i < x_view.size(); ++i) {
    Coord[0][i] = x_view[i];
    Coord[1][i] = y_view[i];
    Coord[2][i] = z_view[i];
  }
}

template <typename TagT>
void
TrilinosInternalLinearSolver<TagT>::updateParallelMng(
    Arccore::MessagePassing::IMessagePassingMng* pm)
{
  m_parallel_mng = pm;
}

template <typename TagT>
bool
TrilinosInternalLinearSolver<TagT>::solve(
    const CSRMatrixType& matrix, const CSRVectorType& b, CSRVectorType& x)
{
  typename TrilinosInternal::SolverInternal<TagT>::Status status =
      m_trilinos_solver->solve(*matrix.internal()->m_internal, *b.internal()->m_internal,
          *x.internal()->m_internal);
  m_status.succeeded = status.m_converged;
  m_status.iteration_count = status.m_num_iters;
  m_status.residual = status.m_residual;
  return m_status.succeeded;
}
/*---------------------------------------------------------------------------*/

template <typename TagT>
void
TrilinosInternalLinearSolver<TagT>::end()
{
  if (m_output_level > 0) {
    internalPrintInfo();
  }
}

template <typename TagT>
const Alien::SolverStatus&
TrilinosInternalLinearSolver<TagT>::getStatus() const
{
  if (m_output_level > 0) {
    printInfo();
  }
  return m_status;
}

template <typename TagT>
void
TrilinosInternalLinearSolver<TagT>::internalPrintInfo() const
{
  m_stater.print(const_cast<ITraceMng*>(traceMng()), m_status,
      Arccore::String::format("Linear Solver : {0}", "TrilinosSolver"));
}

template <typename TagT>
std::shared_ptr<ILinearAlgebra>
TrilinosInternalLinearSolver<TagT>::algebra() const
{
  return std::shared_ptr<ILinearAlgebra>(new Alien::TrilinosLinearAlgebra());
}

template class TrilinosInternalLinearSolver<BackEnd::tag::tpetraserial>;

IInternalLinearSolver<TrilinosMatrix<Real, BackEnd::tag::tpetraserial>,
    TrilinosVector<Real, BackEnd::tag::tpetraserial>>*
TrilinosInternalLinearSolverFactory(
    Arccore::MessagePassing::IMessagePassingMng* p_mng, IOptionsTrilinosSolver* options)
{
  return new TrilinosInternalLinearSolver<BackEnd::tag::tpetraserial>(p_mng, options);
}

#ifdef KOKKOS_ENABLE_OPENMP
template class TrilinosInternalLinearSolver<BackEnd::tag::tpetraomp>;

IInternalLinearSolver<TrilinosMatrix<Real, BackEnd::tag::tpetraomp>,
    TrilinosVector<Real, BackEnd::tag::tpetraomp>>*
TpetraOmpInternalLinearSolverFactory(IParallelMng* p_mng, IOptionsTrilinosSolver* options)
{
  return new TrilinosInternalLinearSolver<BackEnd::tag::tpetraomp>(p_mng, options);
}
#endif

#ifdef KOKKOS_ENABLE_THREADS
template class TrilinosInternalLinearSolver<BackEnd::tag::tpetrapth>;

IInternalLinearSolver<TrilinosMatrix<Real, BackEnd::tag::tpetrapth>,
    TrilinosVector<Real, BackEnd::tag::tpetrapth>>*
TpetraPthInternalLinearSolverFactory(IParallelMng* p_mng, IOptionsTrilinosSolver* options)
{
  return new TrilinosInternalLinearSolver<BackEnd::tag::tpetrapth>(p_mng, options);
}
#endif

#ifdef KOKKOS_ENABLE_CUDA
template class TrilinosInternalLinearSolver<BackEnd::tag::tpetracuda>;

IInternalLinearSolver<TrilinosMatrix<Real, BackEnd::tag::tpetracuda>,
    TrilinosVector<Real, BackEnd::tag::tpetracuda>>*
TpetraCudaInternalLinearSolverFactory(
    IParallelMng* p_mng, IOptionsTrilinosSolver* options)
{
  return new TrilinosInternalLinearSolver<BackEnd::tag::tpetracuda>(p_mng, options);
}
#endif

} // namespace Alien
