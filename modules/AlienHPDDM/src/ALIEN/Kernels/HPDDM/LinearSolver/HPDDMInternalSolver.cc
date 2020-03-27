// -*- C++ -*-
/* Author : couletj at Thu Jul 25 09:24:07 2019
 * Generated by createNew
 */

/* INFO: Les services et modules se conforment maintenant � la politique Arcane core.
 *       L'emploi des .h �tait fictif. 
 *       D�sormais les d�clarations de classe pour les modules et services seront
 *       localis�es dans le fichier .cc correspondant.
 */

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#define MPICH_SKIP_MPICXX 1
#include "mpi.h"

#include <vector>
#include <list>


#include "ALIEN/Kernels/HPDDM/HPDDMPrecomp.h"


#include <ALIEN/Data/Space.h>
#include <ALIEN/Expression/Solver/ILinearSolver.h>
#include <ALIEN/Expression/Solver/ILinearAlgebra.h>
#include <ALIEN/Expression/Solver/SolverStats/SolverStat.h>
#include <ALIEN/Expression/Solver/SolverStats/SolverStater.h>

#include <ALIEN/Kernels/SimpleCSR/SimpleCSRPrecomp.h>
#include <ALIEN/Kernels/SimpleCSR/Algebra/SimpleCSRLinearAlgebra.h>
#include <ALIEN/Kernels/SimpleCSR/DataStructure/SimpleCSRMatrix.h>

//#include <ALIEN/Kernels/HPDDM/Algebra/HPDDMLinearAlgebra.h>
#include <ALIEN/Core/Impl/MultiMatrixImpl.h>
#include <ALIEN/Core/Impl/MultiVectorImpl.h>


//#include <ALIEN/Kernels/HPDDM/Algebra/HPDDMLinearAlgebra.h>
//#include <ALIEN/Kernels/HPDDM/LinearSolver/HPDDMInternalLinearSolver.h>
#include <ALIEN/Core/Backend/LinearSolverT.h>
#include <ALIEN/Core/Block/ComputeBlockOffsets.h>
#include <ALIEN/axl/HPDDMSolver_IOptions.h>

#include <ALIEN/Kernels/HPDDM/DataStructure/HPDDMInternal.h>
#include <ALIEN/Kernels/HPDDM/LinearSolver/HPDDMInternalSolver.h>
/*---------------------------------------------------------------------------*/

namespace Alien {

// Compile HPDDMLinearSolver.
//template class ALIEN_IFPENSOLVERS_EXPORT LinearSolver<BackEnd::tag::htssolver>;


/*---------------------------------------------------------------------------*/
HPDDMInternalSolver::HPDDMInternalSolver(Arccore::MessagePassing::IMessagePassingMng* parallel_mng,
                                         IOptionsHPDDMSolver* options)
  : m_parallel_mng(parallel_mng)
  , m_options(options)
{

}


void
HPDDMInternalSolver::init(int argc, char const** argv)
{
}

void
HPDDMInternalSolver::init()
{
  m_output_level = m_options->outputLevel() ;
  //Parsing des options HPDDM
  HPDDM::Option& opt = *HPDDM::Option::get();
  opt.parse("-hpddm_orthogonalization mgs");
  std::string optstring(
      "-hpddm_verbosity "      + std::to_string(m_options->outputLevel()) +
      " -hpddm_max_it "        + std::to_string(m_options->maxIterationNum()) +
      " -hpddm_gmres_restart " + std::to_string(m_options->maxRestartIterationNum()) +
      " -hpddm_krylov_method " + localstr(m_options->krylovMethod()) );
  opt.parse(optstring);
  std::stringstream stream;
  stream << "-hpddm_tol " << m_options->stopCriteriaValue();
  std::getline(stream, optstring);
  opt.parse(optstring);
  if (m_options->geneoNu() > 0)
  {
    optstring = ("-hpddm_schwarz_coarse_correction balanced -hpddm_geneo_nu " + std::to_string(m_options->geneoNu()));
    opt.parse(optstring);
  }
  if (m_options->cmdLineParam().size() > 0)
  {
    std::stringstream optstream;
    optstream << m_options->cmdLineParam()[0];
    std::getline(optstream, optstring);
    opt.parse(optstring);
  }

  if (m_parallel_mng->commRank() != 0)
    opt.remove("verbosity");
}


void
HPDDMInternalSolver::end()
{
  if(m_output_level>0)
    internalPrintInfo() ;
}

#ifdef ALIEN_USE_HPDDM

void HPDDMInternalSolver::_computeHPDDMRhs(CSRMatrixType const& A, CSRVectorType const& b)
{
  using namespace Arccore ;
  m_hpddm_rhs.resize(m_hpddm_matrix.getNDofs()) ;
  auto values = b.values() ;
  for(auto i=0;i<values.size();++i)
  {
    m_hpddm_rhs[i] = values[i] ;
  }
  Real*       x_ptr = m_hpddm_rhs.data() ;
  b.resize(m_hpddm_matrix.getNDofs()) ; // Need extra memory space for ghost
  Real const* b_ptr = b.getDataPtr() ;
  auto const& dist_info = A.getDistStructInfo() ;
  SimpleCSRInternal::SendRecvOp<Real> op(b_ptr, dist_info.m_send_info,A.getSendPolicy(),
                                         x_ptr, dist_info.m_recv_info,A.getRecvPolicy(),
                                         m_parallel_mng, nullptr);
  op.start();
  op.end() ;
}

void
HPDDMInternalSolver::_computeHPDDMSol(CSRMatrixType const& A, CSRVectorType const& x)
{
  using namespace Arccore ;

  m_hpddm_sol.resize(m_hpddm_matrix.getNDofs()) ;
  auto values = x.values() ;
  for(auto i=0;i<values.size();++i)
  {
    m_hpddm_sol[i] = values[i] ;
  }
  Real*       y_ptr = m_hpddm_sol.data() ;

  x.resize(m_hpddm_matrix.getNDofs()) ; // Need extra memory space for ghost
  Real const* x_ptr = x.getDataPtr() ;
  auto const& dist_info = A.getDistStructInfo() ;
  SimpleCSRInternal::SendRecvOp<Real> op(x_ptr, dist_info.m_send_info,A.getSendPolicy(),
                                         y_ptr, dist_info.m_recv_info,A.getRecvPolicy(),
                                         m_parallel_mng, nullptr);
  op.start();
  op.end() ;
}

void HPDDMInternalSolver::_computeSol(CSRVectorType& x)
{
  auto values = x.values() ;
  for(auto i=0;i<values.size();++i)
  {
    values[i] = m_hpddm_sol[i] ;
  }
}

bool HPDDMInternalSolver::solve(CSRMatrixType const& A,
                                CSRVectorType const& b,
                                CSRVectorType& x)
{
  if(m_output_level>0)
    alien_info([&] { cout()<<"HPDDMSolver::solve"; } ) ;
  {
    Alien::SolverStater::Sentry s(m_init_solver_time) ;

  HPDDM::Option& opt = *HPDDM::Option::get();
  bool schwarz_coarse_correction = opt.set("schwarz_coarse_correction") ;
  m_hpddm_matrix.compute(m_parallel_mng,
                         A,
                         opt["geneo_nu"],
                         schwarz_coarse_correction) ;

  _computeHPDDMRhs(A,b) ;
  _computeHPDDMSol(A,x) ;

  m_status.iteration_count = HPDDM::IterativeMethod::solve(m_hpddm_matrix.matrix(),
                                                           m_hpddm_rhs.data(),
                                                           m_hpddm_sol.data(),
                                                           1,
                                                           m_hpddm_matrix.matrix().getCommunicator());
  }

  HPDDMValueType storage[2];
  m_hpddm_matrix.matrix().computeResidual(m_hpddm_sol.data(), m_hpddm_rhs.data(), storage, 1);
  m_status.residual = storage[1];

  _computeSol(x) ;

  m_status.succeeded = m_status.iteration_count<m_options->maxIterationNum() ;

  return m_status.succeeded ;
}

bool HPDDMInternalSolver::solve(CSRMatrixType const& Ad,
                                CSRMatrixType const& An,
                                CSRVectorType const& b,
                                CSRVectorType& x)
{
  if(m_output_level>0)
    alien_info([&] { cout()<<"HPDDMSolver::solve"; } ) ;

  HPDDM::Option& opt = *HPDDM::Option::get();
  bool schwarz_coarse_correction = opt.set("schwarz_coarse_correction") ;
  m_hpddm_matrix.compute(m_parallel_mng,
                         Ad,An,
                         opt["geneo_nu"],
                         schwarz_coarse_correction) ;

  _computeHPDDMRhs(Ad,b) ;
  _computeHPDDMSol(Ad,x) ;
  }

  {
    Alien::SolverStater::Sentry s(m_iter_solver_time) ;
    m_status.iteration_count = HPDDM::IterativeMethod::solve(m_hpddm_matrix.matrix(),
                                                           m_hpddm_rhs.data(),
                                                           m_hpddm_sol.data(),
                                                           1,
                                                           m_hpddm_matrix.matrix().getCommunicator());
  }

  HPDDMValueType storage[2];
  m_hpddm_matrix.matrix().computeResidual(m_hpddm_sol.data(), m_hpddm_rhs.data(), storage, 1);
  m_status.residual = storage[1];

  _computeSol(x) ;

  m_status.succeeded = m_status.iteration_count<m_options->maxIterationNum() ;

  return m_status.succeeded ;
}
#endif // ALIEN_USE_HPDDM

/*---------------------------------------------------------------------------*/

void
HPDDMInternalSolver::
internalPrintInfo() const
{
  m_stater.print(const_cast<ITraceMng*>(traceMng()), m_status, format("Linear Solver : {0}","HPDDMSolver"));
  if(m_output_level>0)
      alien_info([&] {
                       cout()<<"INIT SOLVER TIME : "<<m_init_solver_time;
                       cout()<<"ITER SOLVER TIME : "<<m_iter_solver_time;
                     }
                ) ;
}


bool
HPDDMInternalSolver::
solve(IMatrix const& A, IVector const& b, IVector& x)
{
  using namespace Alien;

#ifdef ALIEN_USE_HPDDM
    SolverStatSentry<HPDDMInternalSolver> sentry(this,SolverStater::ePrepare) ;
    CSRMatrixType const& matrix = A.impl()->get<BackEnd::tag::simplecsr>() ;
    CSRVectorType const& rhs = b.impl()->get<BackEnd::tag::simplecsr>() ;
    CSRVectorType& sol = x.impl()->get<BackEnd::tag::simplecsr>(true) ;
    sentry.release() ;

    SolverStatSentry<HPDDMInternalSolver> sentry2(this,SolverStater::eSolve) ;
    return solve(matrix,rhs,sol) ;
#else
    return false ;
#endif
}

bool
HPDDMInternalSolver::
solve(IMatrix const& Ad,IMatrix const& An, IVector const& b, IVector& x)
{
  using namespace Alien;

#ifdef ALIEN_USE_HPDDM
    SolverStatSentry<HPDDMInternalSolver> sentry(this,SolverStater::ePrepare) ;
    CSRMatrixType const& matrixD = Ad.impl()->get<BackEnd::tag::simplecsr>() ;
    CSRMatrixType const& matrixN = An.impl()->get<BackEnd::tag::simplecsr>() ;
    CSRVectorType const& rhs = b.impl()->get<BackEnd::tag::simplecsr>() ;
    CSRVectorType& sol = x.impl()->get<BackEnd::tag::simplecsr>(true) ;
    sentry.release() ;

    SolverStatSentry<HPDDMInternalSolver> sentry2(this,SolverStater::eSolve) ;
    return solve(matrixD,matrixN,rhs,sol) ;
#else
    return false ;
#endif
}

ILinearSolver*
HPDDMInternalSolverFactory(Arccore::MessagePassing::IMessagePassingMng* p_mng, IOptionsHPDDMSolver* options)
{
  return new HPDDMInternalSolver(p_mng, options);
}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

}

